// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 */

#include <linux/module.h>
#include "ktf.h"

/*
 * A trivial and somewhat rough example used by the author
 * for pedagogical purposes, to demonstrate
 * interactive debugging with kgdb.
 *
 * Requires a kernel built with CONFIG_KGDB
 *
 * Note: these test breaks into kgdb and/or creates a NULL
 *       pointer exception and corresponding stack dump, so
 *       try out in a test environment only!
 */

MODULE_LICENSE("GPL");

KTF_INIT();

#define MAX_CNT 3
#include <linux/kgdb.h>

static int kgdemo_cnt;
static int *bogus_ref;

TEST(kgdb, breakpoint)
{
	kgdemo_cnt = 0;
	printk(KERN_INFO "** Please set kgdemo_cnt = 1 **\n");
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
