#include <linux/kallsyms.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include "ktf.h"
#include "ktf_map.h"
#include "ktf_cov.h"

static void ktf_cov_entry_free(struct ktf_map_elem *elem)
{
	struct ktf_cov_entry *entry = container_of(elem, struct ktf_cov_entry,
						   kmap);
	DM(T_DEBUG, printk(KERN_INFO "Unregistering probe for %s\n",
	   entry->name));
	unregister_kprobe(&entry->kprobe);
	kfree(entry);
}

static int ktf_cov_entry_compare(const char *key1, const char *key2)
{
	unsigned long k1 = *((unsigned long *)key1);
	unsigned long k2 = *((unsigned long *)key2);

	if (k1 < k2)
		return -1;
	if (k1 > k2)
		return 1;
	return 0;
}

void ktf_cov_entry_put(struct ktf_cov_entry *entry)
{
	ktf_map_elem_put(&entry->kmap);
}

/* Global map for address-> symbol/module mapping */
DEFINE_KTF_MAP(cov_entry_map, ktf_cov_entry_compare, ktf_cov_entry_free);

struct ktf_cov_entry *ktf_cov_entry_find(unsigned long addr)
{
	return ktf_map_find_entry(&cov_entry_map, (char *)&addr,
				  struct ktf_cov_entry, kmap);
}

static void ktf_cov_free(struct ktf_map_elem *elem)
{
	struct ktf_cov *cov = container_of(elem, struct ktf_cov, kmap);

	kfree(cov);
}

void ktf_cov_put(struct ktf_cov *cov)
{
	ktf_map_elem_put(&cov->kmap);
}

/* Coverage object map. Just modules supported for now, sort by name. */
DEFINE_KTF_MAP(cov_map, NULL, ktf_cov_free);

struct ktf_cov *ktf_cov_find(const char *module)
{
	return ktf_map_find_entry(&cov_map, module, struct ktf_cov, kmap);
}

/* Do not use ktf_cov_entry_find() here as we can get entry directly
 * from probe address (as probe is first field in struct ktf_cov_entry).
 * No reference counting issues should apply as when entry refcnt drops
 * to 0 we unregister the kprobe prior to freeing the entry.
 */
static int ktf_cov_handler(struct kprobe *p, struct pt_regs *regs)
{
	struct ktf_cov_entry *entry = (struct ktf_cov_entry *)p;

	/* Make sure probe is ours... */
	if (!entry || entry->magic != KTF_COV_ENTRY_MAGIC)
		return 0;
	entry->count++;
	if (entry->count == 1 && entry->cov)
		entry->cov->count++;
	return 0;
}

static int ktf_cov_init_symbol(void *data, const char *name,
			struct module *mod, unsigned long addr)
{
	struct ktf_cov_entry *entry;
	struct ktf_cov *cov = data;

	if (!mod || !cov)
		return 0;

	/* We only care about symbols for cov-specified module. */
	if (strcmp(mod->name, cov->kmap.key))
		return 0;

	/* We don't probe ourselves and functions called within probe ctxt. */
	if (strncmp(name, "ktf_cov", strlen("ktf_cov")) == 0 ||
	    strcmp(name, "ktf_map_find") == 0)
		return 0;

	/* Check if we're already covered for this module/symbol. */
	entry = ktf_cov_entry_find(addr);
	if (entry) {
		entry->refcnt++;
		ktf_cov_entry_put(entry);
		return 0;
	}
	entry = kzalloc(sizeof(struct ktf_cov_entry), GFP_KERNEL);
	(void) strlcpy(entry->name, name, sizeof(entry->name));
	entry->magic = KTF_COV_ENTRY_MAGIC;
	entry->cov = cov;

	entry->kprobe.pre_handler = ktf_cov_handler;
	entry->kprobe.symbol_name = entry->name;

	/* Ugh - we try to register a kprobe as a means of determining
	 * if the symbol is a function.
	 */
	if (register_kprobe(&entry->kprobe) < 0) {
		/* not a probe-able function */
		kfree(entry);
		return 0;
	}
	entry->refcnt = 1;
	if (ktf_map_elem_init(&entry->kmap, (char *)&entry->kprobe.addr) < 0 ||
	    ktf_map_insert(&cov_entry_map, &entry->kmap) < 0) {
		unregister_kprobe(&entry->kprobe);
		kfree(entry);
		return 0;
	}
	DM(T_DEBUG,
	   printk(KERN_INFO "Added %s/%s (%p) to coverage",
		   mod->name, entry->name, (void *)entry->kprobe.addr));

	cov->total++;
	ktf_cov_entry_put(entry);
	return 0;
}

static int ktf_cov_init(const char *name, enum ktf_cov_type type)
{
	struct ktf_cov *cov = kzalloc(sizeof(struct ktf_cov), GFP_KERNEL);

	if (!cov)
		return -ENOMEM;

	cov->type = type;
	if (ktf_map_elem_init(&cov->kmap, name) < 0 ||
	    ktf_map_insert(&cov_map, &cov->kmap) < 0) {
		kfree(cov);
		return -ENOMEM;
	}
	kallsyms_on_each_symbol(ktf_cov_init_symbol, cov);

	return 0;
}

int ktf_cov_enable(const char *module)
{
	struct ktf_cov *cov = ktf_cov_find(module);

	if (!cov) {
		if (ktf_cov_init(module, KTF_COV_TYPE_MODULE))
			return -ENOMEM;
	}
	return 0;
}

void ktf_cov_disable(const char *module)
{
	struct ktf_cov *cov = ktf_cov_find(module);
	struct ktf_cov_entry *entry;

	if (!cov)
		return;

	ktf_map_for_each_entry(entry, &cov_entry_map, kmap) {
		if (entry->cov != cov)
			continue;
		if (entry->refcnt < 1) {
			printk(KERN_INFO "Reference count for %s/%s < 1",
			       module, entry->name);
			continue;
		}
		if (--entry->refcnt == 0)
			unregister_kprobe(&entry->kprobe);
	}
	ktf_cov_put(cov);
}

void ktf_cov_seq_print(struct seq_file *seq)
{
	struct ktf_cov_entry *entry;
	struct ktf_cov *cov;

	seq_printf(seq, "%20s %44s %10s\n", "MODULE", "#FUNCTIONS",
		   "#CALLED");
	ktf_map_for_each_entry(cov, &cov_map, kmap)
		seq_printf(seq, "%20s %44d %10d\n",
			   cov->kmap.key, cov->total, cov->count);

	seq_printf(seq, "\n%20s %44s %10s\n", "MODULE", "FUNCTION", "COUNT");
	ktf_map_for_each_entry(entry, &cov_entry_map, kmap)
		seq_printf(seq, "%20s %44s %10d\n",
			   entry->cov ? entry->cov->kmap.key : "-",
			   entry->name, entry->count);
}

void ktf_cov_cleanup(void)
{
	struct ktf_cov *cov;
	char name[KTF_MAX_KEY];

	ktf_map_for_each_entry(cov, &cov_map, kmap)
		ktf_cov_disable(ktf_map_elem_name(&cov->kmap, name));
	ktf_map_delete_all(&cov_map);
	ktf_map_delete_all(&cov_entry_map);
}
