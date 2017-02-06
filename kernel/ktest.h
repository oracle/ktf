#ifndef _KTEST_H
#define _KTEST_H

#include <linux/completion.h>
#include "kcheck.h"

struct test_dev {
	struct list_head dev_list; /* Link into list of registered devices */
	struct ib_device *ibdev;  /* Referenced device */
	u8* vpci_base; /* Start of "our" vpci test CSR space */
	char dma_space[4096];	/* 4K 'static' DMA test space.. */
	dma_addr_t dma_adr;
	struct pci_pool *dma_pool;	/* A test DMA pool */
	char *dma_space2; /* Allocated from pool */
	char * base_adr;  /* Used by dma_rw */
	dma_addr_t dma_adr2;
	struct completion intr_compl;
	u32 intr_enabled; /* Set to nonzero if we expect an interrupt */
	u32 intr_count; /* Number of interrupts seen */
	u32 assert_cnt; /* Temporary count of passed assertions */
	u64 min_resp_ticks;   /* expected min. hw resp.time in ticks */
};


typedef void (*test_adder)(void);

/* Generic setup function for client modules */
void ktest_add_tests(test_adder f);

struct test_dev* ktest_number_to_dev(int devno);
struct test_dev* ktest_find_dev(struct ib_device* dev);

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

#define EXPECT_STREQ(X, Y) _ck_assert_str_eq(X, ==, Y)
#define EXPECT_STRNE(X, Y) (!_ck_assert_str_eq(X, !=, Y))

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
				   "siftest pid [%d] " "%s: " format "\n", \
				   current->pid, __func__, \
				   ## arg); \
	} while (0)

#endif
