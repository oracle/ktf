#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include "ktf.h"
#include "ktf_map.h"

enum ktf_cov_type {
	KTF_COV_TYPE_MODULE,
	KTF_COV_TYPE_MAX,
};

struct ktf_cov {
	struct ktf_map_elem kmap;
	enum ktf_cov_type type;		/* only modules supported for now. */
	int count;			/* number of unique functions called */
	int total;			/* total number of functions */
};

#define	KTF_COV_ENTRY_MAGIC		0xc07e8a5e
struct ktf_cov_entry {
	struct kprobe kprobe;
	int magic;			/* magic number identifying entry */
	struct ktf_map_elem kmap;
	char name[KTF_MAX_KEY];
	struct ktf_cov *cov;
	int refcnt;
	int count;
};

struct ktf_cov_entry *ktf_cov_entry_find(unsigned long);
void ktf_cov_entry_put(struct ktf_cov_entry *);
void ktf_cov_entry_get(struct ktf_cov_entry *);

struct ktf_cov *ktf_cov_find(const char *);
void ktf_cov_put(struct ktf_cov *);
void ktf_cov_get(struct ktf_cov *);
void ktf_cov_seq_print(struct seq_file *);
void ktf_cov_cleanup(void);

int ktf_cov_enable(const char *);
void ktf_cov_disable(const char *);
