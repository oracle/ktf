// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 */

#include <linux/module.h>
#include "ktf.h"

MODULE_LICENSE("GPL");

KTF_INIT();

static int count;
static int ret;

KTF_ENTRY_PROBE(printk, printkhandler)
{
	count++;

	KTF_ENTRY_PROBE_RETURN(0);
}

TEST(examples, entrycheck)
{
	count = 0;
	ASSERT_INT_EQ_GOTO(KTF_REGISTER_ENTRY_PROBE(printk, printkhandler),
			   0, done);
	printk(KERN_INFO "Testing kprobe entry...");
	ASSERT_INT_GT_GOTO(count, 0, done);
done:
	KTF_UNREGISTER_ENTRY_PROBE(printk, printkhandler);
}

KTF_RETURN_PROBE(printk, printkrethandler)
{
	ret = KTF_RETURN_VALUE();

	return 0;
}

TEST(examples, returncheck)
{
	char *teststr = "Testing kprobe return...";

	ret = -1;
	ASSERT_INT_EQ_GOTO(KTF_REGISTER_RETURN_PROBE(printk, printkrethandler),
			   0, done);
	printk(KERN_INFO "%s", teststr);
	ASSERT_INT_EQ_GOTO(ret, strlen(teststr), done);
done:
	KTF_UNREGISTER_RETURN_PROBE(printk, printkrethandler);
}

static int __init hello_init(void)
{
	ADD_TEST(entrycheck);
	ADD_TEST(returncheck);
	return 0;
}

static void __exit hello_exit(void)
{
	KTF_CLEANUP();
}

module_init(hello_init);
module_exit(hello_exit);
