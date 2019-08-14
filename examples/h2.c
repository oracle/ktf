// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 */

#include <linux/module.h>
#include "ktf.h"

MODULE_LICENSE("GPL");

KTF_INIT();

#define MAX_CNT 3

struct hello_ctx {
	struct ktf_context k;
	int value[MAX_CNT];
};

static struct hello_ctx myctx = { .value = { 0, 1, 4 } };

TEST(examples, cmp)
{
	struct hello_ctx *hctx = KTF_CONTEXT_GET("value", struct hello_ctx);

	EXPECT_INT_EQ(_i, hctx->value[_i]);
}

static void add_tests(void)
{
	ADD_LOOP_TEST(cmp, 0, MAX_CNT);
}

static int __init hello_init(void)
{
	KTF_CONTEXT_ADD(&myctx.k, "value");
	add_tests();
	return 0;
}

static void __exit hello_exit(void)
{
	struct ktf_context *kctx = KTF_CONTEXT_FIND("value");

	KTF_CONTEXT_REMOVE(kctx);
	KTF_CLEANUP();
}

module_init(hello_init);
module_exit(hello_exit);
