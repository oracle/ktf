#include <linux/module.h>
#include <linux/kallsyms.h>
#include <rdma/ib_verbs.h>
#include "ktest.h"
#include "nl.h"

MODULE_LICENSE("GPL");

ulong ktest_debug_mask = T_INFO;

/* list of all currently registered sif devices */
static LIST_HEAD(dev_list);
DEFINE_SPINLOCK(dev_list_lock);

module_param_named(debug_mask, ktest_debug_mask, ulong, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL(ktest_debug_mask);

/* Defined in kcheck.c */
void ktest_cleanup_check(void);

static void ktest_device_add(struct ib_device* dev)
{
	struct test_dev* tdev;
	unsigned long flags;

	printk ("ktest: added device %s (at %p)\n", dev->name, dev);
	tdev = (struct test_dev*) kmalloc(sizeof(struct test_dev), GFP_KERNEL);
	memset(tdev, 0, sizeof(struct test_dev));
	tdev->min_resp_ticks = 1000;
	tdev->ibdev = dev;
	if (!tdev) {
		printk("ERROR: ktest: failed to allocate test memory\n");
		return;
	}

	spin_lock_irqsave(&dev_list_lock, flags);
	list_add(&tdev->dev_list, &dev_list);
	spin_unlock_irqrestore(&dev_list_lock, flags);
}

#if (KERNEL_VERSION(4, 4, 0) < LINUX_VERSION_CODE)
static void ktest_device_remove(struct ib_device* dev, void *client_data)
#else
static void ktest_device_remove(struct ib_device* dev)
#endif
{
	struct test_dev *pos, *n;
	unsigned long flags;

	/* ktest_find_dev might be called from interrupt level */
	spin_lock_irqsave(&dev_list_lock,flags);
	list_for_each_entry_safe(pos, n, &dev_list, dev_list) {
		if (pos->ibdev == dev) {
			list_del(&pos->dev_list);
			kfree(pos);
			break;
		}
	}
	spin_unlock_irqrestore(&dev_list_lock,flags);
	printk ("ktest: removed device %p\n", dev);
}

static struct ib_client ib_client = {
	.name = "ktest",
	.add = ktest_device_add,
	.remove = ktest_device_remove
};



struct ktest_kernel_internals {
	/* From module.h: Look up a module symbol - supports syntax module:name */
	unsigned long (*module_kallsyms_lookup_name)(const char *);
};

static struct ktest_kernel_internals ki;


static int __init ktest_init(void)
{
	const char* ks = "module_kallsyms_lookup_name";

	/* Register with IB */
	int ret = 0; //ib_register_client(&ib_client);
	if (ret) {
		printk("ktest: Failed to register with IB, ret = %d\n", ret);
		return ret;
	}

	/* We rely on being able to resolve this symbol for looking up module
	 * specific internal symbols (multiple modules may define the same symbol):
	 */
	ki.module_kallsyms_lookup_name = (void*)kallsyms_lookup_name(ks);
	if (!ki.module_kallsyms_lookup_name) {
		printk(KERN_ERR "Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
			ks);
		return -EINVAL;
	}

	ret = ktest_nl_register();
	if (ret) {
		printk(KERN_ERR "Unable to register protocol with netlink");
		goto failure;
	}

	/* NB! Test classes must be inserted alphabetically */
	tcase_create("any");
	tcase_create("mlx");
	tcase_create("prm");
	tcase_create("rtl");

	/* long tests not part of checkin regression */
	tcase_create("rtlx");

	tcase_create("sif");
	return 0;
failure:
	//ib_unregister_client(&ib_client);
	return ret;
}


static void __exit ktest_exit(void)
{
	ktest_cleanup_check();
	//ib_unregister_client(&ib_client);
	ktest_nl_unregister();
}


/* Generic setup function for client modules */
void ktest_add_tests(test_adder f)
{
	f();
}
EXPORT_SYMBOL(ktest_add_tests);


struct test_dev* ktest_find_dev(struct ib_device* dev)
{
	int i = 0;
	struct test_dev* ret = NULL;
	struct test_dev *pos;
	unsigned long flags;

	spin_lock_irqsave(&dev_list_lock, flags);
	list_for_each_entry(pos, &dev_list, dev_list) {
		if (pos->ibdev == dev) {
			ret = pos;
			break;
		}
		i++;
	}
	spin_unlock_irqrestore(&dev_list_lock, flags);
	return ret;
}
EXPORT_SYMBOL(ktest_find_dev);

struct test_dev* ktest_number_to_dev(int devno)
{
	int i = 0;
	struct test_dev* ret = NULL;
	struct test_dev *pos;
	unsigned long flags;

	spin_lock_irqsave(&dev_list_lock, flags);
	list_for_each_entry(pos, &dev_list, dev_list) {
		if (i == devno) {
			ret = pos;
			break;
		}
		i++;
	}
	spin_unlock_irqrestore(&dev_list_lock, flags);
	return ret;
}
EXPORT_SYMBOL(ktest_number_to_dev);


/* Support for looking up module internal symbols to enable testing */
void* ktest_find_symbol(const char *mod, const char *sym)
{
	char sm[200];
	const char *symref;
	unsigned long addr;

	if (mod) {
		sprintf(sm, "%s:%s", mod, sym);
		symref = sm;
	} else
		symref = sym;

	addr = ki.module_kallsyms_lookup_name(symref);
	if (addr)
		tlog(T_INFO, "Found %s at %0lx\n", sym, addr);
	else {
		tlog(T_INFO, "Fatal error: %s not found\n", sym);
		return NULL;
	}
	return (void*)addr;
}
EXPORT_SYMBOL(ktest_find_symbol);


module_init(ktest_init);
module_exit(ktest_exit);
