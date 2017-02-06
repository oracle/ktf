#include <net/netlink.h>
#include <net/genetlink.h>
#define NL_INTERNAL 1
#include "unlproto.h"
#include "kcheck.h"
#include "nl.h"
#include "ktest.h"

/* Generic netlink support to communicate with user level
 * test framework.
 */


/* family definition */
static struct genl_family ktest_gnl_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = "ktest",
	.version = 1,
	.maxattr = KTEST_A_MAX+4
};

/* Callback functions defined below */
static int ktest_run(struct sk_buff *skb, struct genl_info *info);
static int ktest_query(struct sk_buff *skb, struct genl_info *info);

/* handler, returns 0 on success, negative
 * values on failure
 */
static int ktest_req(struct sk_buff *skb, struct genl_info *info)
{
	enum ktest_cmd_type type;
	/* Dispatch on type of request */

	if (!info->attrs[KTEST_A_TYPE]) {
		printk(KERN_ERR "received netlink msg with no type!");
		return -EINVAL;
	}
	type = nla_get_u32(info->attrs[KTEST_A_TYPE]);

	switch(type) {
	case KTEST_CT_QUERY:
		return ktest_query(skb, info);
	case KTEST_CT_RUN:
		return ktest_run(skb, info);
	default:
		printk(KERN_ERR "received netlink msg with invalid type (%d)",
			type);
	}
	return -EINVAL;
}


/* Send data about one testcase */
static int send_test_data(struct sk_buff *resp_skb, TCase *tc)
{
	struct nlattr *nest_attr;
	struct fun_hook *fh;

	int stat = 0;
	stat = nla_put_string(resp_skb, KTEST_A_STR, tc->name);
	if (stat) return stat;
	nest_attr = nla_nest_start(resp_skb, KTEST_A_TEST);
	if (stat) return stat;
	list_for_each_entry(fh, &tc->fun_list, flist) {
		if (fh->fun) {
			stat = nla_put_string(resp_skb, KTEST_A_STR, fh->name);
			if (stat) return stat;
		}
	}
	nla_nest_end(resp_skb, nest_attr);
	return 0;
}



static int ktest_query(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *resp_skb;
	void *data;
	int retval = 0;
	struct nlattr *nest_attr;
	int i;

	/* No options yet, just send a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	data = genlmsg_put_reply(resp_skb, info, &ktest_gnl_family,
				0, KTEST_C_RESP);
	if (data == NULL) {
		retval = -ENOMEM;
		goto resp_failure;
	}
	/* Add all test sets to the report
	 *  We send test info as follows:
	 *    KTEST_CT_QUERY testset_num [testset1 [name1 name2 ..] testset2 [name1 name2 ..]]
	 */
	if (!nla_put_u32(resp_skb, KTEST_A_TYPE, KTEST_CT_QUERY)) {
		nla_put_u32(resp_skb, KTEST_A_NUM, check_test_cnt);
		nest_attr = nla_nest_start(resp_skb, KTEST_A_LIST);
		if (!nest_attr) {
			retval = -ENOMEM;
			goto resp_failure;
		}
		for (i = 0; i < check_test_cnt; i++) {
			send_test_data(resp_skb, &check_test_case[i]);
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



static int ktest_run_funcs(struct sk_buff *skb, struct test_dev* tdev, int setnum, int testnum, u32 value)
{
	TCase* testset;

	/* Execute test functions */
	struct fun_hook *fh;
	int i;
	int tn = 1;
	testset = &check_test_case[setnum];

	list_for_each_entry(fh, &testset->fun_list, flist) {
		if (testnum == 0 || testnum == tn) {
			/* If testnum != 0 run that test only */
			if (fh->fun) {
			        DM(T_DEBUG, printk(KERN_INFO "Running test %s.%s [%d:%d]\n",
					fh->tclass,fh->name, fh->start, fh->end));
				for (i = fh->start; i < fh->end; i++) {
					fh->fun(skb,tdev,i,value);
					flush_assert_cnt(skb);
				}
			} else
				DM(T_DEBUG, printk(KERN_INFO "** no function for test %s.%s **\n",
					fh->tclass,fh->name));
		}
		tn++;
	}
	DM(T_DEBUG, printk(KERN_INFO "Set %s contained %d tests\n",
				testset->name, tn-1));
	return 0;
}


static int ktest_run(struct sk_buff *skb, struct genl_info *info)
{
	int testnum, setnum, devno;
	u32 value = 0;
	struct sk_buff *resp_skb;
	void *data;
	int retval = 0;
	struct nlattr *nest_attr;
	struct test_dev* tdev;

	if (!info->attrs[KTEST_A_DEVNO])	{  /* DEVNO denotes device number */
		printk(KERN_ERR "received KTEST_CT_RUN msg without devno!\n");
		return -EINVAL;
	}
	devno = nla_get_u32(info->attrs[KTEST_A_DEVNO]);
	tdev = ktest_number_to_dev(devno);
	if (!tdev) {
		printk(KERN_ERR "Could not find device index %d\n", devno);
		return -ENODEV;
	}

	if (!info->attrs[KTEST_A_SN])	{  /* Using SN field as testset number */
		printk(KERN_ERR "received KTEST_CT_RUN msg without testset number!\n");
		return -EINVAL;
	}
	setnum = nla_get_u32(info->attrs[KTEST_A_SN]);

	if (!info->attrs[KTEST_A_NUM])	{  /* Using NUM field as test number */
		printk(KERN_ERR "received KTEST_CT_RUN msg without testnum!\n");
		return -EINVAL;
	}
	testnum = nla_get_u32(info->attrs[KTEST_A_NUM]);

	if (info->attrs[KTEST_A_STAT])	{
		/* Using STAT field as optional u32 input parameter to test */
		value = nla_get_u32(info->attrs[KTEST_A_STAT]);
	}

	DM(T_DEBUG, printk(KERN_INFO "ktest_run: Request for testset# %d/%d, "
				"test %d on device %d\n",
				setnum, check_test_cnt, testnum, devno));

	/* Start building a response */
	resp_skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!resp_skb)
		return -ENOMEM;

	data = genlmsg_put_reply(resp_skb, info, &ktest_gnl_family,
				0, KTEST_C_REQ);
	if (data == NULL) {
		retval = -ENOMEM;
		goto put_fail;
	}

	nla_put_u32(resp_skb, KTEST_A_TYPE, KTEST_CT_RUN);

	if (setnum < check_test_cnt && setnum >= 0) {
		nla_put_u32(resp_skb, KTEST_A_NUM, testnum);
		nest_attr = nla_nest_start(resp_skb, KTEST_A_LIST);
		ktest_run_funcs(resp_skb, tdev, setnum, testnum, value);
		nla_nest_end(resp_skb, nest_attr);
	}

	/* Recompute message header */
	genlmsg_end(resp_skb, data);

	retval = genlmsg_reply(resp_skb, info);
	if (!retval)
		DM(T_DEBUG, printk(KERN_INFO "ktest_run: Sent reply for test %d\n",
					testnum));
	else
		printk(KERN_INFO
			"ktest_run: Failed to send reply"
			" for test %d - value %d\n",
			testnum, retval);
put_fail:
	/* Free buffer if failure */
	if (retval)
		nlmsg_free(resp_skb);
	return retval;
}



static int ktest_resp(struct sk_buff *skb, struct genl_info *info)
{
	/* not to expect this message here */
	printk(KERN_INFO "unexpected netlink RESP msg received");
	return 0;
}

/* operation definition */
static struct genl_ops ktest_ops[] = {
	{
		.cmd = KTEST_C_REQ,
		.flags = 0,
		.policy = ktest_gnl_policy,
		.doit = ktest_req,
		.dumpit = NULL,
	},
	{
		.cmd = KTEST_C_RESP,
		.flags = 0,
		.policy = ktest_gnl_policy,
		.doit = ktest_resp,
		.dumpit = NULL,
	}
};


int ktest_nl_register(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,13,7))
	int stat = genl_register_family_with_ops(
		&ktest_gnl_family,
		ktest_ops, ARRAY_SIZE(ktest_ops));
#else
	int stat = genl_register_family_with_ops(&ktest_gnl_family, ktest_ops);
#endif
	return stat;
}

void ktest_nl_unregister(void)
{
	genl_unregister_family(&ktest_gnl_family);
}
