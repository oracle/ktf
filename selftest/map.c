#include <linux/module.h>
#include "ktf.h"
#include "ktf_map.h"
#include "ktf_syms.h"

MODULE_LICENSE("GPL");

struct map_test_ctx {
	struct ktf_context k;
};

struct map_test_ctx s_mctx[3];

/* Declare a simple handle with no contexts for simple (unparameterized) tests: */
KTF_INIT();

/* For tests that defines multiple test cases
 * (e.g. if the test scope requires application of each test on several devices or
 *  other abstract contexts, definable by the test module)
 */
KTF_HANDLE_INIT(dual_handle);
KTF_HANDLE_INIT(single_handle);
KTF_HANDLE_INIT(no_handle);
KTF_HANDLE_INIT_VERSION(wrongversion_handle, 0);

struct map_test_ctx *to_mctx(struct ktf_context *ctx)
{
	return container_of(ctx, struct map_test_ctx, k);
}

struct myelem {
	struct ktf_map_elem foo;
	int freed;
};

/* should be called when refcount is 0. */
void myelem_free(struct ktf_map_elem *elem)
{
	struct myelem *myelem = container_of(elem, struct myelem, foo);
	myelem->freed = 1;
}

TEST(any, simplemap)
{
	int i;
	const int nelems = 3;
	struct map_test_ctx *mctx = to_mctx(ctx);
	struct ktf_map tm, tm2;
	struct myelem e[nelems], *ep;
	struct ktf_map_elem *elem;

	if (mctx) {
		tlog(T_DEBUG, "ctx %s\n", mctx->k.elem.name);
	} else
		tlog(T_DEBUG, "ctx <none>\n");

	ktf_map_init(&tm, NULL);
	EXPECT_INT_EQ(0, ktf_map_elem_init(&e[0].foo, "foo"));
	EXPECT_INT_EQ(0, ktf_map_elem_init(&e[1].foo, "bar"));
	EXPECT_INT_EQ(0, ktf_map_elem_init(&e[2].foo, "zax"));

	for (i = 0; i < nelems; i++) {
		EXPECT_LONG_EQ(i, ktf_map_size(&tm));
		EXPECT_INT_EQ(0, ktf_map_insert(&tm, &e[i].foo));
	}
	EXPECT_LONG_EQ(i, ktf_map_size(&tm));

	/* Should be sorted alphabetically so we get 'bar' back: */
	EXPECT_ADDR_EQ(&e[1].foo, ktf_map_find_first(&tm));

	for (i = 0; i < nelems; i++) {
		EXPECT_LONG_EQ(nelems - i, ktf_map_size(&tm));
		EXPECT_ADDR_EQ(&e[i].foo, ktf_map_remove(&tm, e[i].foo.name));
	}
	EXPECT_LONG_EQ(0, ktf_map_size(&tm));

	/* Reference counting test */
	ktf_map_init(&tm2, myelem_free);
	/* Init map elems with "foo" "bar" "zax" */
	EXPECT_INT_EQ(0, ktf_map_elem_init(&e[0].foo, "foo"));
	EXPECT_INT_EQ(0, ktf_map_elem_init(&e[1].foo, "bar"));
	EXPECT_INT_EQ(0, ktf_map_elem_init(&e[2].foo, "zax"));

	/* Insert elems and drop our refcounts (map still holds ref) */
	for (i = 0; i < nelems; i++) {
		EXPECT_INT_EQ(0, ktf_map_insert(&tm2, &e[i].foo));
		ktf_map_elem_put(&e[i].foo);
	}

	/* This macro takes (and drops) refcount for each elem */
	ktf_map_for_each_entry(ep, &tm2, foo)
		ep->freed = 0;

	for (i = 0; i < nelems; i++) {
		elem = ktf_map_remove(&tm2, e[i].foo.name);
		EXPECT_INT_EQ(0, e[i].freed);
		/* free our ref, now free function should be called. */
		ktf_map_elem_put(elem);
		EXPECT_INT_EQ(1, e[i].freed);
	}
}

TEST(any, dummy)
{
	EXPECT_TRUE(true);
}

TEST(any, wrongversion)
{
	tlog(T_INFO, "This test should never have run - wrong version\n!!!");
	EXPECT_TRUE(false);
}

static void add_map_tests(void)
{
	ADD_TEST(dummy);
	ADD_TEST_TO(dual_handle, simplemap);
	/* This should fail */
	ADD_TEST_TO(wrongversion_handle, wrongversion);
}


static int __init maptest_init(void)
{
	int ret = ktf_context_add(&dual_handle, &s_mctx[0].k, "map1");
	if (ret)
		return ret;

	ret = ktf_context_add(&dual_handle, &s_mctx[1].k, "map2");
	if (ret)
		return ret;

	ret = ktf_context_add(&single_handle, &s_mctx[2].k, "map3");
	if (ret)
		return ret;

	resolve_symbols();

	add_map_tests();
	tlog(T_INFO, "maptest: loaded\n");
	return 0;
}

static void __exit maptest_exit(void)
{
	KTF_HANDLE_CLEANUP(single_handle);
	KTF_HANDLE_CLEANUP(dual_handle);
	KTF_HANDLE_CLEANUP(no_handle);
	KTF_CLEANUP();
	tlog(T_INFO, "map: unloaded\n");
	/* Nothing to do here */
}


module_init(maptest_init);
module_exit(maptest_exit);
