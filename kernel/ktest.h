#ifndef _KTEST_H
#define _KTEST_H

#include <linux/completion.h>
#include "kcheck.h"
#include "ktest_map.h"

struct ktest_context {
	struct ktest_map_elem elem;  /* Linkage for ktest_map */
	struct ktest_handle *handle; /* Owner of this context */
};

typedef void (*ktest_test_adder)(void);

/* Generic setup function for client modules */
void ktest_add_tests(ktest_test_adder f);

int ktest_context_add(struct ktest_handle *handle, struct ktest_context* ctx, const char* name);
struct ktest_context* ktest_find_context(struct ktest_handle *handle, const char* name);
struct ktest_context *ktest_find_first_context(struct ktest_handle *handle);
struct ktest_context *ktest_find_next_context(struct ktest_context* ctx);
void ktest_context_remove(struct ktest_context *ctx);

static inline size_t ktest_has_contexts(struct ktest_handle *handle) {
	return ktest_map_size(&handle->ctx_map) > 0;
}

/* Declare the implicit __test_handle as extern for .c files that use it
 * when adding tests with ADD_TEST but where definition is in another .c file:
 */
extern struct ktest_handle __test_handle;

/* Add/remove/find a context to/from the default handle */
#define KTEST_CONTEXT_ADD(__context, name) ktest_context_add(&__test_handle, __context, name)
#define KTEST_CONTEXT_REMOVE(__context) ktest_context_remove(__context)
#define KTEST_CONTEXT_FIND(name) ktest_find_context(&__test_handle, name)

/* Test macros */

/* Asserts that also makes the current function return; */
#define ASSERT_TRUE(C) { \
	if (!fail_unless((C))) return;	\
}

#define ASSERT_FALSE(C) { \
	if (!fail_unless(!(C))) return;	\
}

#define ASSERT_TRUE_GOTO(C,_lbl) {		\
	if (!fail_unless((C))) goto _lbl;\
}

#define ASSERT_FALSE_GOTO(C,_lbl) {		\
	if (!fail_unless(!(C))) goto _lbl;\
}

#define ASSERT_LONG_EQ(X, Y) \
	_ck_assert_long_ret(X, ==, Y);

#define ASSERT_LONG_NE(X, Y) \
	_ck_assert_long_ret(X, !=, Y);

#define ASSERT_ADDR_EQ(X, Y) \
	_ck_assert_long_ret((u64)(X), ==, (u64)(Y));

#define ASSERT_ADDR_NE(X, Y) \
	_ck_assert_long_ret((u64)(X), !=, (u64)(Y));

#define ASSERT_INT_EQ(X, Y) \
	_ck_assert_int_ret(X, ==, Y);

#define ASSERT_INT_GT(X, Y) \
	_ck_assert_int_ret(X, >, Y);

#define ASSERT_LONG_EQ_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto(X, ==, Y, _lbl)

#define ASSERT_LONG_NE_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto(X, !=, Y, _lbl)

#define ASSERT_ADDR_EQ_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto((u64)(X), ==, (u64)(Y), _lbl)

#define ASSERT_ADDR_NE_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto((u64)(X), !=, (u64)(Y), _lbl)

#define ASSERT_INT_EQ_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, ==, Y, _lbl)

#define ASSERT_INT_GE_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, >=, Y, _lbl)

#define ASSERT_INT_GT_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, >, Y, _lbl)

#define ASSERT_INT_LT_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, <, Y, _lbl)

#define ASSERT_INT_NE(X,Y) \
	_ck_assert_int_ret(X, !=, Y);

#define ASSERT_INT_NE_GOTO(X,Y,_lbl) \
	_ck_assert_int_goto(X, !=, Y, _lbl);

#define EXPECT_TRUE(C) { \
	fail_unless(C); \
}

#define EXPECT_FALSE(C) { \
	fail_unless(!(C));\
}

#define OK_ADDR(X) (X && !IS_ERR(X))

/* Valid kernel address check */
#define EXPECT_OK_ADDR(X) \
	ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - was 0x%Lx", (X))

#define ASSERT_OK_ADDR(X) { \
	if (!ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - value 0x%Lx", (X))) \
		return; \
}
#define ASSERT_OK_ADDR_GOTO(X,_lbl) { \
	if (!ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - was 0x%Lx", (X))) \
		goto _lbl; \
}

#define EXPECT_INT_EQ(X,Y) _ck_assert_int(X, ==, Y)
#define EXPECT_INT_GT(X,Y) _ck_assert_int(X, >, Y)
#define EXPECT_INT_GE(X,Y) _ck_assert_int(X, >=, Y)
#define EXPECT_INT_LE(X,Y) _ck_assert_int(X, <=, Y)
#define EXPECT_INT_LT(X,Y) _ck_assert_int(X, <, Y)
#define EXPECT_INT_NE(X,Y) _ck_assert_int(X, !=, Y)

#define EXPECT_LONG_EQ(X, Y) _ck_assert_long(X, ==, Y)
#define EXPECT_LONG_NE(X, Y) _ck_assert_long(X, !=, Y)
#define EXPECT_ADDR_EQ(X, Y) _ck_assert_long((u64)(X), ==, (u64)(Y))
#define EXPECT_ADDR_NE(X, Y) _ck_assert_long((u64)(X), !=, (u64)(Y))
#define EXPECT_LONG_GT(X, Y) _ck_assert_long(X, >=, Y)

#define EXPECT_STREQ(X, Y) _ck_assert_str_eq(X, Y)
#define EXPECT_STRNE(X, Y) _ck_assert_str_ne(X, Y)

extern ulong ktest_debug_mask;
#define DM(m, x) do { if (ktest_debug_mask & m) { x; } } while (0)

// Defined debug bits:
#define T_ERROR    0x1
#define T_INFO	   0x2
#define T_LIST   0x100
#define T_INTR   0x200
#define T_DEBUG 0x1000
#define T_MCAST 0x2000

#define tlog(class, format, arg...)	\
	do { \
		if (unlikely((ktest_debug_mask) & (class)))	\
			printk(KERN_INFO \
				   "ktest pid [%d] " "%s: " format "\n", \
				   current->pid, __func__, \
				   ## arg); \
	} while (0)


/* Look up the current address of a potentially local symbol - to allow testing
 * against it. NB! This is a hack for unit testing internal unexposed interfaces and
 * violates the module boundaries and has no fw/bw comp gauarantees, but are
 * still very useful for detailed unit testing complex logic:
 */
void* ktest_find_symbol(const char *mod, const char *sym);

#define ktest_resolve_symbol(mname, sname) \
	do { \
		sname = ktest_find_symbol(#mname, #sname);	\
		if (!sname) \
			return -ENOENT; \
	} while (0)
#endif
