// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 *
 * ktf_nl.c: ktf netlink protocol implementation
 */
#include <linux/version.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#define NL_INTERNAL 1
#include "ktf_unlproto.h"
#include "ktf_test.h"
#include "ktf_nl.h"
#include "ktf.h"
#include "ktf_cov.h"
#include "ktf_compat.h"

/* Generic netlink support to communicate with user level
 * test framework.
 */

/* Callback functions defined below */
static int ktf_run(struct sk_buff *skb, struct genl_info *info);
static int ktf_query(struct sk_buff *skb, struct genl_info *info);
static int ktf_cov_cmd(struct sk_buff *skb, struct genl_info *info);
static int ktf_ctx_cfg(struct sk_buff *skb, struct genl_info *info);
static int send_version_only(struct sk_buff *skb, struct genl_info *info);

/* operation definitions - see ktf_unlproto.h for definitions */
static struct genl_ops ktf_ops[] = {
	{
		.cmd = KTF_C_QUERY,
		.flags = 0,
#if (KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE)
		.policy = ktf_gnl_policy,
#endif
		.doit = ktf_query,
	},
	{
		.cmd = KTF_C_RUN,
		.flags = 0,
#if (KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE)
		.policy = ktf_gnl_policy,
#endif
		.doit = ktf_run,
	},
	{
		.cmd = KTF_C_COV,
		.flags = 0,
#if (KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE)
		.policy = ktf_gnl_policy,
#endif
		.doit = ktf_cov_cmd,
	},
	{
		.cmd = KTF_C_CTX_CFG,
		.flags = 0,
#if (KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE)
		.policy = ktf_gnl_policy,
#endif
		.doit = ktf_ctx_cfg,
	}
};

/* family definition */
static struct genl_family ktf_gnl_family = {
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	.id = GENL_ID_GENERATE,
#else
	.module = THIS_MODULE,
#endif
	.hdrsize = 0,
	.name = "ktf",
	.version = 1,
	.maxattr = KTF_A_MAX + 4,
#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
	.policy = ktf_gnl_policy,
#endif
#if (KERNEL_VERSION(3, 13, 7) < LINUX_VERSION_CODE)
	.ops = ktf_ops,
	.n_ops = ARRAY_SIZE(ktf_ops),
#endif
};

static int check_version(enum ktf_cmd cmd, struct sk_buff *skb, struct genl_info *info)
{
	u64 version;

	if (!info->attrs[KTF_A_VERSION]) {
		terr("received netlink msg with no version!");
		return -EINVAL;
	}

	version = nla_get_u64(info->attrs[KTF_A_VERSION]);
	if (ktf_version_check(version)) {
		/* a query is the first call for any reasonable application:
		 * Respond to it with a version only:
		 */
		if (cmd == KTF_C_QUERY)
			return send_version_only(skb, info);
		return -EINVAL;
	}
	return 0;
}

/* Reply with just version information to let user space report the issue: */
static int send_version_only(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	void *data;
	int retval = 0;

	if (!resp_skb)
		return -ENOMEM;
	data = genlmsg_put_reply(resp_skb, info, &ktf_gnl_family,
				 0, KTF_C_QUERY);
	if (!data) {
		retval = -ENOMEM;
		goto resp_failure;
	}
	nla_put_u64_64bit(resp_skb, KTF_A_VERSION, KTF_VERSION_LATEST, 0);

	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
resp_failure:
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}

/* Send data about one testcase */
static int send_test_data(struct sk_buff *resp_skb, struct ktf_case *tc)
{
	struct nlattr *nest_attr;
	struct ktf_test *t;
	int stat;
	int cnt = 0;

	stat = nla_put_string(resp_skb, KTF_A_STR, ktf_case_name(tc));
	if (stat)
		return stat;

	nest_attr = nla_nest_start(resp_skb, KTF_A_TEST);
	ktf_testcase_for_each_test(t, tc) {
		cnt++;
		/* A test is not valid if the handle requires a context and none is present */
		if (t->handle->id) {
			stat = nla_put_u32(resp_skb, KTF_A_HID, t->handle->id);
			if (stat)
				goto fail;
		} else if (t->handle->require_context) {
			continue;
		}
		stat = nla_put_string(resp_skb, KTF_A_STR, t->name);
		if (stat)
			goto fail;
	}
	nla_nest_end(resp_skb, nest_attr);
	tlog(T_DEBUG, "Sent data about %d tests", cnt);
	return 0;
fail:
	twarn("Failed with status %d after sending data about %d tests", stat, cnt);
	/* we hold reference to t here - drop it! */
	ktf_test_put(t);
	return stat;
}

static int send_handle_data(struct sk_buff *resp_skb, struct ktf_handle *handle)
{
	struct ktf_context_type *ct;
	struct nlattr *nest_attr;
	struct ktf_context *ctx;
	int stat;

	tlog(T_DEBUG, "Sending context handle %d: ", handle->id);

	/* Send HID */
	stat = nla_put_u32(resp_skb, KTF_A_HID, handle->id);
	if (stat)
		return stat;

	/* Send contexts */
	nest_attr = nla_nest_start(resp_skb, KTF_A_LIST);
	if (!nest_attr)
		return -ENOMEM;

	tlog(T_DEBUG, "Sending context type list");
	/* Send any context types that user space are allowed to create contexts for */
	ktf_map_for_each_entry(ct, &handle->ctx_type_map, elem) {
		if (ct->alloc) {
			stat = nla_put_string(resp_skb, KTF_A_FILE, ct->name);
			if (stat)
				return -ENOMEM;
		}
	}

	/* Then send all the contexts themselves */
	ctx = ktf_find_first_context(handle);
	while (ctx) {
		nla_put_string(resp_skb, KTF_A_STR, ktf_context_name(ctx));
		if (ctx->config_cb) {
			stat = nla_put_string(resp_skb, KTF_A_MOD, ctx->type->name);
			if (stat)
				return stat;
			stat = nla_put_u32(resp_skb, KTF_A_STAT, ctx->config_errno);
			if (stat)
				return stat;
		}
		ctx = ktf_find_next_context(ctx);
	}
	nla_nest_end(resp_skb, nest_attr);
	return 0;
}

static int ktf_query(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *resp_skb;
	void *data;
	int retval = 0;
	struct nlattr *nest_attr;
	struct ktf_handle *handle;
	struct ktf_case *tc;

	retval = check_version(KTF_C_QUERY, skb, info);
	if (retval)
		return retval;

	/* No options yet, just send a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	data = genlmsg_put_reply(resp_skb, info, &ktf_gnl_family,
				 0, KTF_C_QUERY);
	if (!data) {
		retval = -ENOMEM;
		goto resp_failure;
	}

	nla_put_u64_64bit(resp_skb, KTF_A_VERSION, KTF_VERSION_LATEST, 0);

	/* Add all test sets to the report
	 *  We send test info as follows:
	 *    KTF_CT_QUERY hid1 [context1 [context2 ...]] hid2 [context1 [context2 ...]]
	 *                   testset_num [testset1 [name1 name2 ..] testset2 [name1 name2 ..]]
	 *  Handle IDs without contexts are not present
	 */
	if (!list_empty(&context_handles)) {
		/* Traverse list of handles with contexts */
		nest_attr = nla_nest_start(resp_skb, KTF_A_HLIST);
		list_for_each_entry(handle, &context_handles, handle_list) {
			retval = send_handle_data(resp_skb, handle);
			if (retval)
				goto resp_failure;
		}
		nla_nest_end(resp_skb, nest_attr);
	}

	/* Send total number of tests */
	tlog(T_DEBUG, "Total #of test cases: %ld", ktf_case_count());
	nla_put_u32(resp_skb, KTF_A_NUM, ktf_case_count());
	nest_attr = nla_nest_start(resp_skb, KTF_A_LIST);
	if (!nest_attr) {
		retval = -ENOMEM;
		goto resp_failure;
	}
	ktf_for_each_testcase(tc) {
		retval = send_test_data(resp_skb, tc);
		if (retval) {
			retval = -ENOMEM;
			goto resp_failure;
		}
	}
	nla_nest_end(resp_skb, nest_attr);

	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
resp_failure:
	if (retval)
		twarn("Message failure (status %d)", retval);
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}

static int ktf_run_func(struct sk_buff *skb, const char *ctxname,
			const char *setname, const char *testname,
			u32 value, void *oob_data, size_t oob_data_sz)
{
	struct ktf_case *testset = ktf_case_find(setname);
	struct ktf_test *t;
	int tn = 0;

	if (!testset) {
		tlog(T_INFO, "No such testset \"%s\"", setname);
		return -EFAULT;
	}

	/* Execute test functions */
	ktf_testcase_for_each_test(t, testset) {
		if (t->fun && strcmp(t->name, testname) == 0) {
			struct ktf_context *ctx = ktf_find_context(t->handle, ctxname);

			ktf_run_hook(skb, ctx, t, value, oob_data, oob_data_sz);
		} else if (!t->fun) {
			tlog(T_DEBUG, "** no function for test %s.%s **", t->tclass, t->name);
		}
		tn++;
	}
	tlog(T_DEBUG, "Set %s contained %d tests", ktf_case_name(testset), tn);
	ktf_case_put(testset);
	return 0;
}

static int ktf_run(struct sk_buff *skb, struct genl_info *info)
{
	u32 value = 0;
	struct sk_buff *resp_skb;
	void *data;
	int retval = 0;
	struct nlattr *nest_attr, *data_attr;
	char ctxname_store[KTF_MAX_NAME + 1];
	char *ctxname = ctxname_store;
	char setname[KTF_MAX_NAME + 1];
	char testname[KTF_MAX_NAME + 1];
	void *oob_data = NULL;
	size_t oob_data_sz = 0;

	retval = check_version(KTF_C_RUN, skb, info);
	if (retval)
		return retval;

	if (info->attrs[KTF_A_STR])
		nla_strscpy(ctxname, info->attrs[KTF_A_STR], KTF_MAX_NAME);
	else
		ctxname = NULL;

	if (!info->attrs[KTF_A_SNAM])	{
		terr("received KTF_CT_RUN msg without testset name!");
		return -EINVAL;
	}
	nla_strscpy(setname, info->attrs[KTF_A_SNAM], KTF_MAX_NAME);

	if (!info->attrs[KTF_A_TNAM])	{  /* Test name wo/context */
		terr("received KTF_CT_RUN msg without test name!");
		return -EINVAL;
	}
	nla_strscpy(testname, info->attrs[KTF_A_TNAM], KTF_MAX_NAME);

	if (info->attrs[KTF_A_NUM])	{
		/* Using NUM field as optional u32 input parameter to test */
		value = nla_get_u32(info->attrs[KTF_A_NUM]);
	}

	data_attr = info->attrs[KTF_A_DATA];
	if (data_attr)	{
		/* User space sends out-of-band data: */
		oob_data = nla_memdup(data_attr, GFP_KERNEL);
		oob_data_sz = nla_len(data_attr);
	}

	tlog(T_DEBUG, "Request for testset %s, test %s", setname, testname);

	/* Start building a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	data = genlmsg_put_reply(resp_skb, info, &ktf_gnl_family,
				 0, KTF_C_RUN);
	if (!data) {
		retval = -ENOMEM;
		goto put_fail;
	}

	nest_attr = nla_nest_start(resp_skb, KTF_A_LIST);
	retval = ktf_run_func(resp_skb, ctxname, setname, testname, value, oob_data, oob_data_sz);
	nla_nest_end(resp_skb, nest_attr);
	nla_put_u32(resp_skb, KTF_A_STAT, retval);

	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
	if (!retval)
		tlog(T_DEBUG, "Sent reply for test %s.%s", setname, testname);
	else
		twarn("Failed to send reply for test %s.%s - value %d",
		      setname, testname, retval);

	kfree(oob_data);
put_fail:
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}

static int ktf_cov_cmd(struct sk_buff *skb,
		       struct genl_info *info)
{
	char *cmd;
	char module[KTF_MAX_NAME + 1];
	struct sk_buff *resp_skb;
	int retval = 0;
	void *data;
	u32 opts = 0;
	bool enable = false;

	retval = check_version(KTF_C_COV, skb, info);
	if (retval)
		return retval;

	if (!info->attrs[KTF_A_MOD])   {
		terr("received KTF_CT_COV msg without module name!");
		return -EINVAL;
	}

	if (info->attrs[KTF_A_NUM])	{
		/* Using NUM field as enable == 1 or disable == 0 */
		enable = nla_get_u32(info->attrs[KTF_A_NUM]);
	}
	cmd = enable ? "enable" : "disable";

	nla_strscpy(module, info->attrs[KTF_A_MOD], KTF_MAX_NAME);
	if (info->attrs[KTF_A_COVOPT])
		opts = nla_get_u32(info->attrs[KTF_A_COVOPT]);

	/* Start building a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	tlog(T_DEBUG, "%s coverage for %s", cmd, module);
	if (enable)
		retval = ktf_cov_enable(module, opts);
	else
		ktf_cov_disable(module);

	data = genlmsg_put_reply(resp_skb, info, &ktf_gnl_family,
				 0, KTF_C_COV);
	if (!data) {
		retval = -ENOMEM;
		goto put_fail;
	}
	nla_put_u32(resp_skb, KTF_A_NUM, enable ? 1 : 0);
	nla_put_u32(resp_skb, KTF_A_STAT, retval);
	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
	if (!retval)
		tlog(T_DEBUG, "Sent reply for %s module %s",
		     cmd, module);
	else
		twarn("Failed to send reply for %s module %s - value %d",
		      cmd, module, retval);
put_fail:
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}

/* Process request to configure a configurable context:
 * Expected format:  KTF_C_CTX_CFG hid type_name context_name data
 * placed in A_HID, A_FILE, A_STR and A_DATA respectively.
 */
static int ktf_ctx_cfg(struct sk_buff *skb, struct genl_info *info)
{
	char ctxname[KTF_MAX_NAME + 1];
	char type_name[KTF_MAX_NAME + 1];
	struct nlattr *data_attr;
	void *ctx_data = NULL;
	size_t ctx_data_sz = 0;
	int hid;
	struct ktf_handle *handle;
	struct ktf_context *ctx;
	int ret;

	ret = check_version(KTF_C_CTX_CFG, skb, info);
	if (ret)
		return ret;

	if (!info->attrs[KTF_A_STR] || !info->attrs[KTF_A_HID])
		return -EINVAL;
	data_attr = info->attrs[KTF_A_DATA];
	if (!data_attr)
		return -EINVAL;
	hid = nla_get_u32(info->attrs[KTF_A_HID]);
	handle = ktf_handle_find(hid);
	if (!handle)
		return -EINVAL;
	if (info->attrs[KTF_A_FILE])
		nla_strscpy(type_name, info->attrs[KTF_A_FILE], KTF_MAX_NAME);
	else
		strcpy(type_name, "default");
	nla_strscpy(ctxname, info->attrs[KTF_A_STR], KTF_MAX_NAME);
	tlog(T_DEBUG, "Trying to find/create context %s with type %s", ctxname, type_name);
	ctx = ktf_find_create_context(handle, ctxname, type_name);
	if (!ctx)
		return -ENODEV;

	tlog(T_DEBUG, "Received context configuration for context %s, handle %d",
	     ctxname, hid);

	ctx_data = nla_memdup(data_attr, GFP_KERNEL);
	ctx_data_sz = nla_len(data_attr);
	ret = ktf_context_set_config(ctx, ctx_data, ctx_data_sz);
	kfree(ctx_data);
	return ret;
}

int ktf_nl_register(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 7))
	int stat = genl_register_family_with_ops(&ktf_gnl_family, ktf_ops,
						 ARRAY_SIZE(ktf_ops));
#else
	int stat = genl_register_family(&ktf_gnl_family);
#endif
	return stat;
}

void ktf_nl_unregister(void)
{
	genl_unregister_family(&ktf_gnl_family);
}
