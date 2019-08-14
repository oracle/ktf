// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 */

#include <linux/module.h>
#include "ktf.h"

MODULE_LICENSE("GPL");

KTF_INIT();

DECLARE_F(hello_fixture)
	struct list_head head;
};

struct my_element {
	struct list_head list;
	int value;
};

SETUP_F(hello_fixture, hello_setup)
{
	int i;

	INIT_LIST_HEAD(&hello_fixture->head);
	for (i = 0; i < 10; i++) {
		struct my_element *e = kzalloc(sizeof(*e), GFP_KERNEL);

		e->value = i;
		list_add_tail(&e->list, &hello_fixture->head);
	}
	hello_fixture->ok = true;
}

TEARDOWN_F(hello_fixture, hello_teardown)
{
	struct list_head *p, *next_p;

	/* Just cleanup whatever is left after the test */
	list_for_each_safe(p, next_p, &hello_fixture->head) {
		struct my_element *e = list_entry(p, struct my_element, list);

		list_del(&e->list);
		kfree(e);
	}
	EXPECT_TRUE(list_empty(&hello_fixture->head));
}

INIT_F(hello_fixture, hello_setup, hello_teardown);

TEST_F(hello_fixture, examples, hello_del)
{
	int cnt = 0;
	int cnt_ones = 0;
	struct my_element *e = kzalloc(sizeof(*e), GFP_KERNEL);

	e->value = 1;
	list_add(&e->list, &ctx->head);

	list_for_each_entry(e, &ctx->head, list) {
		if (e->value == 1)
			cnt_ones++;
		cnt++;
	}
	EXPECT_INT_EQ(11, cnt);
	EXPECT_INT_EQ(2, cnt_ones);
}

static void add_tests(void)
{
	ADD_TEST(hello_del);
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
