// SPDX-License-Identifier: GPL-2.1
/*
 * Copyright (C) 2001, 2002, Arien Malec
 * Copyright (C) 2011, 2017, Oracle and/or its affiliates.
 *
 * This file originates from check.h from the Check C unit test
 * framework, adapted by Knut Omang to build with the linux kernel.
 */

#ifndef KTF_TEST_H
#define KTF_TEST_H

#include <net/netlink.h>
#include <linux/version.h>
#include "ktf_map.h"
#include "ktf_unlproto.h"

/* A test context is an extendable object that a test client module
 * can supply, and that all tests will be invoked with as an implicit
 * 'ctx' argument:
 */
struct ktf_context;

struct ktf_test;

typedef void (*ktf_test_fun) (struct ktf_test *, struct ktf_context* tdev, int, u32);

struct ktf_debugfs {
        struct dentry *debugfs_results_testset;
        struct dentry *debugfs_results_test;
        struct dentry *debugfs_run_testset;
        struct dentry *debugfs_run_test;
};

struct ktf_test {
	struct ktf_map_elem kmap; /* linkage for test case list */
	const char* tclass; /* test class name */
	const char* name; /* Name of the test */
	ktf_test_fun fun;
	int start; /* Start and end value to argument to fun */
	int end;   /* Defines number of iterations */
	struct sk_buff *skb; /* sk_buff for recording assertion results */
	char *log; /* per-test log */
	void *data; /* Test specific out-of-band data */
	size_t data_sz; /* Size of the data element, if set */
	struct timespec64 lastrun; /* last time test was run */
	struct ktf_debugfs debugfs; /* debugfs info for test */
	struct ktf_handle *handle; /* Handler for owning module */
};

struct ktf_case {
	struct ktf_map_elem kmap; /* Linkage for ktf_map */
	struct ktf_map tests; /* List of tests to run */
	struct ktf_debugfs debugfs; /* debugfs handles for testset */
};

/* Used for tests that spawn kthreads to pass state.  We should probably
 * look at passing data to tests like this to make things more extensible,
 * but will defer for now as this would disrupt KTF consumers.
 */
struct ktf_test_state {
	struct ktf_test *self;
	struct ktf_context *ctx;
	int iter;
	u32 value;
};

extern struct ktf_map test_cases;

/* Current total number of test cases defined */
size_t ktf_case_count(void);
const char *ktf_case_name(struct ktf_case *);
/* Manage test case refcount. */
void ktf_case_get(struct ktf_case *);
void ktf_case_put(struct ktf_case *);

int ktf_version_check(u64 version);

void ktf_run_hook(struct sk_buff *skb, struct ktf_context *ctx,
		struct ktf_test *t, u32 value,
		void *oob_data, size_t oob_data_sz);
void flush_assert_cnt(struct ktf_test *self);

/* Representation of a test case (a group of tests) */
struct ktf_case;

struct ktf_case *ktf_case_find(const char *name);

/* Each module client of the test framework is required to
 * declare at least one ktf_handle via the macro
 * DECLARE_KTF_HANDLE (below)
 * If the module require extra data of some sorts, that
 * can be embedded within the handle
 */
struct ktf_handle;

/* Find the handle associated with handle id hid */
struct ktf_handle *ktf_handle_find(int hid);

/* Called upon ktf unload to clean up test cases */
int ktf_cleanup(void);

/* The list of handles that have contexts associated with them */
extern struct list_head context_handles;

struct __test_desc
{
	const char* tclass; /* Test class name */
	const char* name;   /* Test name */
	const char* file;   /* File that implements test */
	ktf_test_fun fun;
};

/* Manage refcount for tests. */
void ktf_test_get(struct ktf_test *t);
void ktf_test_put(struct ktf_test *t);

/* Add a test function to a test case for a given handle (macro version) */
#define ktf_add_test_to(td, __test_handle)					\
	_ktf_add_test(td##_setup, &__test_handle, 0, 0, 0, 1)

/* Add a test function to a test case (macro version) */
#define ktf_add_test(td) \
	_ktf_add_test(td##_setup, &__test_handle, 0, 0, 0, 1)

/* Add a looping test function to a test case (macro version)

   The test will be called in a for(i = s; i < e; i++) loop with each
   iteration being executed in a new context. The loop variable 'i' is
   available in the test.
 */
#define ktf_add_loop_test(td,s,e)				\
	_ktf_add_test(td##_setup, &__test_handle, 0,0,(s),(e))

/* Add a test function to a test case
  (function version -- use this when the macro won't work
*/
void _ktf_add_test(struct __test_desc td, struct ktf_handle *th,
		int _signal, int allowed_exit_value, int start, int end);

/* Internal function to mark the start of a test function */
void ktf_fn_start (const char *fname, const char *file, int line);

/* Add a test previously created with TEST() or TEST_F() */
#define ADD_TEST(__testname)\
	ktf_add_test(__testname)

#define ADD_TEST_TO(__handle, __testname) \
	ktf_add_test_to(__testname, __handle)

#define ADD_LOOP_TEST(__testname, from, to)			\
	ktf_add_loop_test(__testname, from, to)

/* Remove a test previously added with ADD_TEST */
#define DEL_TEST(__testname)\
	ktf_del_test(__testname)

/* Iterate over all test cases.  Implicitly bumps refcount for pos and
 * decreases it after we iterate past it.
 */
#define ktf_for_each_testcase(pos)	\
	ktf_map_for_each_entry(pos, &test_cases, kmap)

/* Iterate over all tests for testcases.  Implicitly bumps refcount for pos
 * and decreases it again after we iterate past it.
 */
#define ktf_testcase_for_each_test(pos, tc)	\
	ktf_map_for_each_entry(pos, &tc->tests, kmap)

#define KTF_GEN_TYPEID_MAX 3

/* A test_handle identifies the calling module:
 * Declare one in the module global scope using
 *  KTF_INIT() or KTF_HANDLE_INIT()
 *  and call KTF_CLEANUP() or KTF_HANDLE_CLEANUP() upon unload
 */

struct ktf_handle {
	struct list_head handle_list; /* Linkage for the global list of all handles with context */
	struct ktf_map ctx_type_map; /* a map from type_id to ktf_context_type (see ktf_context.c) */
	struct ktf_map ctx_map;     /* a (possibly empty) map from name to context for this handle */
	unsigned int id; 	      /* A unique nonzero ID for this handle, set iff contexts */
	bool require_context;	      /* If set, tests are only valid if a context is provided */
	u64 version;		      /* version assoc. with handle */
	struct ktf_test *current_test;/* Current test running */
};

void ktf_test_cleanup(struct ktf_handle *th);
void ktf_handle_cleanup_check(struct ktf_handle *handle);
void ktf_cleanup_check(void);

#define KTF_HANDLE_INIT_VERSION(__test_handle, __version, __need_ctx)	\
	struct ktf_handle __test_handle = { \
		.handle_list = LIST_HEAD_INIT(__test_handle.handle_list), \
		.ctx_type_map = __KTF_MAP_INITIALIZER(__test_handle, NULL, NULL), \
		.ctx_map = __KTF_MAP_INITIALIZER(__test_handle, NULL, NULL), \
		.id = 0, \
		.require_context = __need_ctx, \
		.version = __version, \
	};

#define	KTF_HANDLE_INIT(__test_handle)	\
	KTF_HANDLE_INIT_VERSION(__test_handle, KTF_VERSION_LATEST, false)

#define KTF_INIT() KTF_HANDLE_INIT(__test_handle)

#define	KTF_HANDLE_CTX_INIT(__test_handle)	\
	KTF_HANDLE_INIT_VERSION(__test_handle, KTF_VERSION_LATEST, true)

#define KTF_CTX_INIT() KTF_HANDLE_CTX_INIT(__test_handle)

#define KTF_HANDLE_CLEANUP(__test_handle)	\
	do { \
		ktf_context_remove_all(&__test_handle); \
		ktf_test_cleanup(&__test_handle); \
	} while (0)

#define KTF_CLEANUP() KTF_HANDLE_CLEANUP(__test_handle)

/* Start a unit test with TEST(suite_name,unit_name)
*/
#define TEST(__testsuite, __testname)\
	static void __testname(struct ktf_test *self, struct ktf_context *ctx, \
			int _i, u32 _value);		    \
	struct __test_desc __testname##_setup =			\
        { .tclass = "" # __testsuite "", .name = "" # __testname "",\
	  .fun = __testname, .file = __FILE__ };    \
	\
	static void __testname(struct ktf_test *self, struct ktf_context* ctx, \
			int _i, u32 _value)

/* Start a unit test using a fixture
 * NB! Note the intentionally missing start parenthesis on DECLARE_F!
 *
 *   Prep:
 *      DECLARE_F(fixture_name)
 *            <attributes>
 *      };
 *      INIT_F(fixture_name,setup,teardown);
 *
 *   Usage:
 *      SETUP_F(fixture_name,setup)
 *      {
 *             <setup code, set fixture_name->ok to true to have the test executed>
 *      }
 *      TEARDOWN_F(fixture_name,teardown)
 *      {
 *             <teardown code>
 *      }
 *      TEST_F(fixture_name,suite_name,test_name)
 *      {
 *             <test code>
 *      }
 *
 *   setup must set ctx->ok to true to have the test itself executed
 */
#define DECLARE_F(__fixture) \
	struct __fixture { \
		void (*setup) (struct ktf_test *, struct ktf_context *, struct __fixture *); \
		void (*teardown) (struct ktf_test *, struct __fixture *); \
		bool ok;

#define INIT_F(__fixture,__setup,__teardown) \
	void __setup(struct ktf_test *, struct ktf_context *, struct __fixture *); \
	void __teardown(struct ktf_test *, struct __fixture *); \
	static struct __fixture __fixture##_template = {\
		.setup = __setup, \
		.teardown = __teardown, \
		.ok = false,\
	}

#define	SETUP_F(__fixture, __setup) \
	void __setup(struct ktf_test *self, struct ktf_context *ctx, \
		     struct __fixture *__fixture)

#define	TEARDOWN_F(__fixture, __teardown) \
	void __teardown(struct ktf_test *self, struct __fixture *__fixture)

#define TEST_F(__fixture, __testsuite, __testname) \
	static void __testname##_body(struct ktf_test *, struct __fixture *, \
				      int, u32); \
	static void __testname(struct ktf_test *, struct ktf_context *, int, \
			       u32); \
	struct __test_desc __testname##_setup = \
        { .tclass = "" # __testsuite "", .name = "" # __testname "", \
	  .fun = __testname }; \
	\
	static void __testname(struct ktf_test *self, struct ktf_context* ctx, \
		int _i, u32 _value) \
	{ \
		struct __fixture f_ctx = __fixture##_template; \
		f_ctx.ok = false; \
		f_ctx.setup(self, ctx, &f_ctx); \
		if (!f_ctx.ok) return; \
		__testname##_body(self, &f_ctx, _i, _value); \
		f_ctx.teardown(self,&f_ctx); \
	} \
	static void __testname##_body(struct ktf_test *self, struct __fixture *ctx, \
			int _i, u32 _value)

/* Fail the test case unless expr is true */
/* The space before the comma sign before ## is essential to be compatible
   with gcc 2.95.3 and earlier.
*/
#define ktf_assert_msg(expr, format, ...)			\
	_ktf_assert(self, expr, __FILE__, __LINE__,		\
        format , ## __VA_ARGS__, NULL)

#define ktf_assert(expr, ...)\
	_ktf_assert(self, expr, __FILE__, __LINE__,		\
        "Failure '"#expr"' occurred " , ## __VA_ARGS__, NULL)

/* Always fail */
#define ktf_fail(...) _ktf_assert(self, 0, __FILE__, __LINE__, "Failed" , ## __VA_ARGS__, NULL)

/* Non-macro version of ktf_assert, with more complicated interface
 * returns nonzero if ok, 0 otherwise
 */
long _ktf_assert (struct ktf_test *self, int result, const char *file,
		int line, const char *expr, ...);

/* Integer comparsion macros with improved output compared to ktf_assert(). */
/* O may be any comparion operator. */
#define ktf_assert_int_goto(X, O, Y, _lbl)		\
	do { int x = (X); int y = (Y);\
		if (!ktf_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%x, "#Y"==0x%x", x, y)) \
			goto _lbl;\
	} while (0)

#define ktf_assert_int(X, O, Y) \
	do { int x = (X); int y = (Y);\
		ktf_assert_msg(x O y,\
		  "Assertion '"#X#O#Y"' failed: "#X"==0x%x, "#Y"==0x%x", x, y);\
	} while (0)

#define ktf_assert_int_ret(X, O, Y)\
	do { int x = (X); int y = (Y);\
		if (!ktf_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y))	\
			 return;\
	} while (0)

#define ktf_assert_long_goto(X, O, Y, _lbl)		\
	do { long x = (X); long y = (Y);\
		if (!ktf_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y))	\
			 goto _lbl;\
	} while (0)

#define ktf_assert_long_ret(X, O, Y)\
	do { long x = (X); long y = (Y);\
		if (!ktf_assert_msg(x O y,					\
			"Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y))	\
			 return;\
	} while (0)

/* O may be any comparion operator. */
#define ktf_assert_long(X, O, Y) \
	do { long x = (X); long y = (Y);\
		ktf_assert_msg(x O y,\
		  "Assertion '"#X#O#Y"' failed: "#X"==0x%lx, "#Y"==0x%lx", x, y);\
	} while (0)

/* String comparsion macros with improved output compared to ktf_assert() */
#define ktf_assert_str_eq(X, Y)	\
	do { const char* x = (X); const char* y = (Y);\
		ktf_assert_msg(strcmp(x,y) == 0,\
		  "Assertion '"#X"=="#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\"",\
		  x, y);\
	} while (0)

#define ktf_assert_str_ne(X, Y)				\
	do { const char* x = (X); const char* y = (Y);\
		ktf_assert_msg(strcmp(x,y) != 0,\
		  "Assertion '"#X"!="#Y"' failed: "#X"==\"%s\", "#Y"==\"%s\"",\
		  x, y);\
	} while (0)

#endif /* KTF_TEST_H */
