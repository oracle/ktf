#include <net/netlink.h>
#include <net/genetlink.h>
#define NL_INTERNAL 1
#include "unlproto.h"
#include "kcheck.h"
#include "nl.h"
#include "ktf.h"

/* Generic netlink support to communicate with user level
 * test framework.
 */

/* Callback functions defined below */
static int ktf_run(struct sk_buff *skb, struct genl_info *info);
static int ktf_query(struct sk_buff *skb, struct genl_info *info);
static int ktf_req(struct sk_buff *skb, struct genl_info *info);
static int ktf_resp(struct sk_buff *skb, struct genl_info *info);

/* operation definition */
static struct genl_ops ktf_ops[] = {
	{
		.cmd = KTF_C_REQ,
		.flags = 0,
		.policy = ktf_gnl_policy,
		.doit = ktf_req,
		.dumpit = NULL,
	},
	{
		.cmd = KTF_C_RESP,
		.flags = 0,
		.policy = ktf_gnl_policy,
		.doit = ktf_resp,
		.dumpit = NULL,
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
	.maxattr = KTF_A_MAX+4,
#if (KERNEL_VERSION(3, 13, 7) < LINUX_VERSION_CODE)
	.ops = ktf_ops,
	.n_ops = ARRAY_SIZE(ktf_ops),
#endif
};

/* handler, returns 0 on success, negative
 * values on failure
 */
static int ktf_req(struct sk_buff *skb, struct genl_info *info)
{
	enum ktf_cmd_type type;
	/* Dispatch on type of request */

	if (!info->attrs[KTF_A_TYPE]) {
		printk(KERN_ERR "received netlink msg with no type!");
		return -EINVAL;
	}
	type = nla_get_u32(info->attrs[KTF_A_TYPE]);

	switch(type) {
	case KTF_CT_QUERY:
		return ktf_query(skb, info);
	case KTF_CT_RUN:
		return ktf_run(skb, info);
	default:
		printk(KERN_ERR "received netlink msg with invalid type (%d)",
			type);
	}
	return -EINVAL;
}


/* Send data about one testcase */
static int send_test_data(struct sk_buff *resp_skb, struct ktf_case *tc)
{
	struct nlattr *nest_attr;
	struct fun_hook *fh;
	int stat;

	stat = nla_put_string(resp_skb, KTF_A_STR, tc_name(tc));
	if (stat) return stat;
	nest_attr = nla_nest_start(resp_skb, KTF_A_TEST);
	list_for_each_entry(fh, &tc->fun_list, flist) {
		if (fh->handle->id)
			nla_put_u32(resp_skb, KTF_A_HID, fh->handle->id);
		stat = nla_put_string(resp_skb, KTF_A_STR, fh->name);
		if (stat) return stat;
	}
	nla_nest_end(resp_skb, nest_attr);
	return 0;
}



static int send_handle_data(struct sk_buff *resp_skb, struct ktf_handle *handle)
{
	struct nlattr *nest_attr;
	struct ktf_context *ctx;
	int stat;

	tlog(T_DEBUG, "Found context handle %d: ", handle->id);

	/* Send HID */
	stat = nla_put_u32(resp_skb, KTF_A_HID, handle->id);
	if (stat) return stat;

	/* Send contexts */
	nest_attr = nla_nest_start(resp_skb, KTF_A_LIST);
	if (!nest_attr)
		return -ENOMEM;

	ctx = ktf_find_first_context(handle);
	while (ctx) {
		nla_put_string(resp_skb, KTF_A_STR, ctx->elem.name);
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

	/* No options yet, just send a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	data = genlmsg_put_reply(resp_skb, info, &ktf_gnl_family,
				0, KTF_C_RESP);
	if (data == NULL) {
		retval = -ENOMEM;
		goto resp_failure;
	}
	/* Add all test sets to the report
	 *  We send test info as follows:
	 *    KTF_CT_QUERY hid1 [context1 [context2 ...]] hid2 [context1 [context2 ...]]
	 *                   testset_num [testset1 [name1 name2 ..] testset2 [name1 name2 ..]]
	 *  Handle IDs without contexts are not present
	 */
	if (!nla_put_u32(resp_skb, KTF_A_TYPE, KTF_CT_QUERY)) {
		if (!list_empty(&context_handles)) {
			/* Traverse list of handles with contexts */
			nest_attr = nla_nest_start(resp_skb, KTF_A_HLIST);
			list_for_each_entry(handle, &context_handles, handle_list) {
				send_handle_data(resp_skb, handle);
			}
			nla_nest_end(resp_skb, nest_attr);
		}

		/* Send total number of tests */
		nla_put_u32(resp_skb, KTF_A_NUM, ktf_case_count());
		nest_attr = nla_nest_start(resp_skb, KTF_A_LIST);
		if (!nest_attr) {
			retval = -ENOMEM;
			goto resp_failure;
		}
		ktf_map_for_each_entry(tc, &test_cases, kmap) {
			retval = send_test_data(resp_skb, tc);
			if (retval) {
				retval = -ENOMEM;
				goto resp_failure;
			}
		}
		nla_nest_end(resp_skb, nest_attr);
	}

	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
resp_failure:
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}



static int ktf_run_func(struct sk_buff *skb, const char* ctxname,
			const char *setname, const char *testname, u32 value)
{
	struct ktf_case* testset = ktf_case_find(setname);

	/* Execute test functions */
	struct fun_hook *fh;
	int i;
	int tn = 0;

	if (!testset) {
		tlog(T_INFO, "No such testset \"%s\"\n", setname);
		return -EFAULT;
	}

	list_for_each_entry(fh, &testset->fun_list, flist) {
		if (fh->fun && strcmp(fh->name,testname) == 0) {
			struct ktf_context *ctx = ktf_find_context(fh->handle, ctxname);
			for (i = fh->start; i < fh->end; i++) {
				DM(T_DEBUG,
					printk(KERN_INFO "Running test %s.%s",
						fh->tclass, fh->name);
					if (ctx)
						printk("_%s", ctxname);
					printk("[%d:%d]\n", fh->start, fh->end);
					);
				fh->fun(skb,ctx,i,value);
				flush_assert_cnt(skb);
			}
		} else if (!fh->fun)
			DM(T_DEBUG, printk(KERN_INFO "** no function for test %s.%s **\n",
						fh->tclass,fh->name));
		tn++;
	}
	DM(T_DEBUG, printk(KERN_INFO "Set %s contained %d tests\n",
				tc_name(testset), tn));
	return 0;
}


static int ktf_run(struct sk_buff *skb, struct genl_info *info)
{
	u32 value = 0;
	struct sk_buff *resp_skb;
	void *data;
	int retval = 0;
	struct nlattr *nest_attr;
	char ctxname_store[KTF_MAX_NAME+1];
	char *ctxname = ctxname_store;
	char setname[KTF_MAX_NAME+1];
	char testname[KTF_MAX_NAME+1];

	if (info->attrs[KTF_A_STR]) {
		nla_strlcpy(ctxname, info->attrs[KTF_A_STR], KTF_MAX_NAME);
	} else
		ctxname = NULL;

	if (!info->attrs[KTF_A_SNAM])	{
		printk(KERN_ERR "received KTF_CT_RUN msg without testset name!\n");
		return -EINVAL;
	}
	nla_strlcpy(setname, info->attrs[KTF_A_SNAM], KTF_MAX_NAME);

	if (!info->attrs[KTF_A_TNAM])	{  /* Test name wo/context */
		printk(KERN_ERR "received KTF_CT_RUN msg without test name!\n");
		return -EINVAL;
	}
	nla_strlcpy(testname, info->attrs[KTF_A_TNAM], KTF_MAX_NAME);

	if (info->attrs[KTF_A_NUM])	{
		/* Using NUM field as optional u32 input parameter to test */
		value = nla_get_u32(info->attrs[KTF_A_NUM]);
	}

	tlog(T_DEBUG, "ktf_run: Request for testset %s, test %s\n", setname, testname);

	/* Start building a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	data = genlmsg_put_reply(resp_skb, info, &ktf_gnl_family,
				0, KTF_C_REQ);
	if (data == NULL) {
		retval = -ENOMEM;
		goto put_fail;
	}

	nla_put_u32(resp_skb, KTF_A_TYPE, KTF_CT_RUN);
	nest_attr = nla_nest_start(resp_skb, KTF_A_LIST);
	retval = ktf_run_func(resp_skb, ctxname, setname, testname, value);
	nla_nest_end(resp_skb, nest_attr);
	nla_put_u32(resp_skb, KTF_A_STAT, retval);

	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
	if (!retval)
		tlog(T_DEBUG, "Sent reply for test %s.%s\n", setname, testname);
	else
		printk(KERN_WARNING
			"ktf_run: Failed to send reply for test %s.%s - value %d\n",
			setname, testname, retval);
put_fail:
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}



static int ktf_resp(struct sk_buff *skb, struct genl_info *info)
{
	/* not to expect this message here */
	printk(KERN_INFO "unexpected netlink RESP msg received");
	return 0;
}

int ktf_nl_register(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,13,7))
	int stat = genl_register_family_with_ops(
		&ktf_gnl_family,
		ktf_ops, ARRAY_SIZE(ktf_ops));
#else
	int stat = genl_register_family(&ktf_gnl_family);
#endif
	return stat;
}

void ktf_nl_unregister(void)
{
	genl_unregister_family(&ktf_gnl_family);
}
