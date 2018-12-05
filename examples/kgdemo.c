#include <linux/module.h>
#include "ktf.h"

/* A trivial and somewhat rough example used to demonstrate
 * interactive debugging with kgdb.
 * Requires a kernel built with CONFIG_KGDB
 */

MODULE_LICENSE("GPL");

KTF_INIT();

#define MAX_CNT 3
#include <linux/kgdb.h>

int kgdemo_cnt = 0;
int *bogus_ref = NULL;

TEST(kgdb, breakpoint)
{
	kgdemo_cnt = 0;
	printk("** Please set kgdemo_cnt = 1 **\n");
	kgdb_breakpoint();
	EXPECT_INT_EQ(1, kgdemo_cnt);
}

TEST(kgdb, nullpointer)
{
	int pre = kgdemo_cnt;

	int b = *bogus_ref++;
	EXPECT_INT_EQ(pre + 1, b);
}

static void add_tests(void)
{
	ADD_TEST(breakpoint);
	ADD_TEST(nullpointer);
}


static int __init kgdemo_init(void)
{
	add_tests();
	return 0;
}

static void __exit kgdemo_exit(void)
{
	KTF_CLEANUP();
}


module_init(kgdemo_init);
module_exit(kgdemo_exit);
