// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * context.c: Parameterized context test case, kernel side:
 */

#include "ktf.h"
#include "context.h"

/* Declare a specific handle for this test to avoid interfering with the
 * other tests:
 */
static KTF_HANDLE_INIT(ct_handle);

struct param_test_ctx {
	struct ktf_context k;
	struct test_parameter_block p;
};

struct param_test_ctx param_ctx[2];

#define MYVALUE 0xdabadaba

/* Declare the callback that accepts a parameter block */
static int param_ctx_cb(struct ktf_context *ctx, const void *data, size_t data_sz)
{
	struct param_test_ctx *px = container_of(ctx, struct param_test_ctx, k);
	struct test_parameter_block *pb = (struct test_parameter_block *)data;
	long orig_myvalue;

	if (data_sz != sizeof(*pb))
		return -EINVAL;
	/* check data validity here, if possible.. */
	orig_myvalue = px->p.myvalue;
	memcpy(&px->p, pb, data_sz);
	/* Enforce "policies" */
	px->p.myvalue = orig_myvalue;
	return 0;
}

TEST(selftest, param)
{
	struct param_test_ctx *px = container_of(ctx, struct param_test_ctx, k);

	/* Now, here we can fail (using ASSERT) or ignore by silently return
	 * depending on what's most useful, if a test hasn't been configured.
	 * For this selftest we just use EXPECT so we can have the actual current
	 * parameter values reported as well.
	 *
	 * Notice that these parameters are
	 * persistent throughout the instance 'life' of the kernel test module,
	 * so if one user program has configured them, then
	 * programs ignorant of the parameters may still end up
	 * executing the tests with previously configured parameters:
	 *
	 * This simplified example uses the same configuration struct for both
	 * context type IDs, but the idea is that they can be completely different.
	 */
	EXPECT_INT_EQ(ctx->config_errno, 0);
	if (KTF_CONTEXT_CFG_OK(ctx)) {
		switch (ctx->type->name[13]) {
		case '1':
			EXPECT_LONG_EQ(px->p.magic, CONTEXT_MAGIC1);
			break;
		case '2':
			EXPECT_LONG_EQ(px->p.magic, CONTEXT_MAGIC2);
			break;
		case '3':
			EXPECT_LONG_EQ(px->p.magic, CONTEXT_MAGIC3);
			EXPECT_LONG_EQ(px->p.myvalue, MYVALUE);
			break;
		}
		EXPECT_STREQ(px->p.s, CONTEXT_MSG);
	} else {
		EXPECT_LONG_EQ(px->p.magic, 0);
		EXPECT_STREQ(px->p.s, "");
	}
}

struct param_test_type {
	struct ktf_context_type kt;
	/* space for cfg data (such as constraints) for the context type */
	long myvalue;
};

static struct ktf_context *type3_alloc(struct ktf_context_type *ct)
{
	struct param_test_type *pct = container_of(ct, struct param_test_type, kt);
	struct param_test_ctx *ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);

	ctx->p.myvalue = pct->myvalue;
	return &ctx->k;
}

static void type3_cleanup(struct ktf_context *ctx)
{
	struct param_test_ctx *px = container_of(ctx, struct param_test_ctx, k);

	kfree(px);
}

TEST(selftest, dupltype)
{
	/* Verify that we cannot add the same context type twice */

	static struct param_test_type dupltype = {
		.myvalue = 0,
		.kt.alloc = type3_alloc,
		.kt.config_cb = param_ctx_cb,
		.kt.cleanup = type3_cleanup,
		.kt.name = "context_type_3"
	};

	ASSERT_INT_EQ(-EEXIST, ktf_handle_add_ctx_type(&ct_handle, &dupltype.kt));
}

void add_context_tests(void)
{
	int ret = KTF_CONTEXT_ADD_TO_CFG(ct_handle, &param_ctx[0].k, "context1",
					 param_ctx_cb, "context_type_1");

	if (ret)
		return;

	ret = KTF_CONTEXT_ADD_TO_CFG(ct_handle, &param_ctx[1].k, "context2",
				     param_ctx_cb, "context_type_2");
	if (ret)
		return;

	{
		static struct param_test_type ctx_type3 = {
			.myvalue = MYVALUE,
			.kt.alloc = type3_alloc,
			.kt.config_cb = param_ctx_cb,
			.kt.cleanup = type3_cleanup,
			.kt.name = "context_type_3"
		};
		ret = ktf_handle_add_ctx_type(&ct_handle, &ctx_type3.kt);
	}

	ADD_TEST_TO(ct_handle, param);
	ADD_TEST(dupltype);
}

void context_tests_cleanup(void)
{
	KTF_HANDLE_CLEANUP(ct_handle);
}
