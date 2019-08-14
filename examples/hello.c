// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 */

#include <linux/module.h>
#include "ktf.h"

MODULE_LICENSE("GPL");

KTF_INIT();

TEST(examples, hello_ok)
{
	EXPECT_TRUE(true);
}

TEST(examples, hello_fail)
{
	EXPECT_TRUE(false);
}

static void add_tests(void)
{
	ADD_TEST(hello_ok);
	ADD_TEST(hello_fail);
}

static int __init hello_init(void)
{
	add_tests();
	tlog(T_INFO, "hello: loaded");
	return 0;
}

static void __exit hello_exit(void)
{
	KTF_CLEANUP();
	tlog(T_INFO, "hello: unloaded");
}

module_init(hello_init);
module_exit(hello_exit);
