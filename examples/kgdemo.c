#include <linux/module.h>
#include "ktf.h"

MODULE_LICENSE("GPL");

KTF_INIT();

#define MAX_CNT 3
#include <linux/kgdb.h>

struct kgdb_demo_ctx {
	struct ktf_context k;
	int cnt;
};
struct kgdb_demo_ctx myctx = {
	.cnt = 0,
};

struct kgdb_demo_ctx *to_hctx(struct ktf_context *ctx)
{
	return container_of(ctx, struct kgdb_demo_ctx, k);
}

TEST(kgdb, breakpoint)
{
	printk("** Please set myctx.cnt = 1 **\n");
	kgdb_breakpoint();
	EXPECT_INT_EQ(1, myctx.cnt);
}

static void add_tests(void)
{
  ADD_TEST(breakpoint);
}


static int __init kgdemo_init(void)
{
	KTF_CONTEXT_ADD(&myctx.k, "kgdemo");
	add_tests();
	return 0;
}

static void __exit kgdemo_exit(void)
{
	struct ktf_context *kctx = KTF_CONTEXT_FIND("kgdemo");
	KTF_CONTEXT_REMOVE(kctx);
	KTF_CLEANUP();
}


module_init(kgdemo_init);
module_exit(kgdemo_exit);
