// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * hybrid.c: Hybrid (combined user level and kernel) self tests, kernel side
 */

#include "ktf.h"
#include "hybrid.h"

/* First a simple message passing test that just verifies that we receive
 * "out-of-band" data from user space:
 */

TEST(selftest, msg)
{
	/* Accept data of type 'struct hybrid_self_params' (defined in hybrid_self.h)
	 * from user mode. This functionality is to allow user mode to test something,
	 * for instance that a certain parameter is handled in a specific way in the kernel.
	 * The user then has the option to provide data to the kernel out-of-band to
	 * tell the kernel side what to expect.
	 * In this test, just verify that data has been transmitted correctly:
	 */
	KTF_USERDATA(self, hybrid_self_params, data);

	EXPECT_STREQ(data->text_val, HYBRID_MSG);
	EXPECT_LONG_EQ(data->val, HYBRID_MSG_VAL);
}

void add_hybrid_tests(void)
{
	ADD_TEST(msg);
}
