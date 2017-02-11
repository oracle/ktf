#include <linux/module.h>
#include "ktest.h"
#include "ktest_map.h"
#include <linux/kallsyms.h>

struct ktest_syms {
	void (*map_init)(struct ktest_map *map);
	int (*elem_init)(struct ktest_elem *elem, const char *name);
};

struct ktest_syms ktest_syms;

#define ktest_map_init ktest_syms.map_init
#define ktest_elem_init ktest_syms.elem_init


MODULE_LICENSE("GPL");

TEST_INIT_HANDLE();

TEST(any, simplemap)
{
	struct ktest_map tm;
	struct myelem {
		struct ktest_elem foo;
	};

	struct myelem e[2];

	ktest_map_init(&tm);
	ASSERT_INT_EQ(0, ktest_elem_init(&e[0].foo, "myelem1"));
	ASSERT_INT_EQ(0, ktest_elem_init(&e[1].foo, "myelem2"));
}

static void add_map_tests(void)
{
	ADD_TEST(simplemap);
}


static int resolve_symbols(void)
{
	ktest_resolve_symbol(ktest, ktest_map_init);
	ktest_resolve_symbol(ktest, ktest_elem_init);
	return 0;
}


static int __init maptest_init(void)
{
	struct ktest_map tm;
	resolve_symbols();
	ktest_map_init(&tm);
	//add_map_tests();
	tlog(T_INFO, "maptest: loaded\n");
	return 0;
}

static void __exit maptest_exit(void)
{
	TEST_CLEANUP();
	tlog(T_INFO, "map: unloaded\n");
	/* Nothing to do here */
}


module_init(maptest_init);
module_exit(maptest_exit);
