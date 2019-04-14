/* Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0
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
		switch (ctx->config_type) {
		case CONTEXT1_TYPE_ID:
			EXPECT_LONG_EQ(px->p.magic, CONTEXT_MAGIC1);
			break;
		case CONTEXT2_TYPE_ID:
			EXPECT_LONG_EQ(px->p.magic, CONTEXT_MAGIC2);
			break;
		case CONTEXT3_TYPE_ID:
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

struct param_test_type ctx_type3;

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

void add_context_tests(void)
{
	int ret = KTF_CONTEXT_ADD_TO_CFG(ct_handle, &param_ctx[0].k, "context1",
					 param_ctx_cb, CONTEXT1_TYPE_ID);

	if (ret)
		return;

	ret = KTF_CONTEXT_ADD_TO_CFG(ct_handle, &param_ctx[1].k, "context2",
				     param_ctx_cb, CONTEXT2_TYPE_ID);
	if (ret)
		return;

	ctx_type3.myvalue = MYVALUE;
	ret = ktf_handle_add_ctx_type(&ct_handle, &ctx_type3.kt,
				      type3_alloc, param_ctx_cb, type3_cleanup,
				      CONTEXT3_TYPE_ID);

	ADD_TEST_TO(ct_handle, param);
}

void context_tests_cleanup(void)
{
	ktf_context_remove_all(&ct_handle);
	KTF_HANDLE_CLEANUP(ct_handle);
}
