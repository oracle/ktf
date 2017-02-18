#ifndef _KTEST_UNLPROTO_H
#define _KTEST_UNLPROTO_H
#ifdef __cplusplus
extern "C" {
#endif

enum ktest_cmd_type {
	KTEST_CT_UNSPEC,
	KTEST_CT_QUERY,
	KTEST_CT_RUN,
	KTEST_CT_MAX,
};

/* Netlink protocol definition shared between user and kernel space
 * Include once per user app as it defines struct values!
 */

/* supported attributes */
enum ktest_attr {
	KTEST_A_UNSPEC,
	KTEST_A_TYPE,
	KTEST_A_SNAM,
	KTEST_A_TNAM,
	KTEST_A_NUM,
	KTEST_A_STR,
	KTEST_A_FILE,
	KTEST_A_STAT,
	KTEST_A_LIST,
	KTEST_A_TEST,
	KTEST_A_HID,    /* Test handle ID */
	KTEST_A_HLIST,  /* List of handles repr. as a LIST of contexts for a given HID */
	KTEST_A_MAX
};

/* attribute policy */
#ifdef NL_INTERNAL
struct nla_policy ktest_gnl_policy[KTEST_A_MAX] = {
	[KTEST_A_TYPE]  = { .type = NLA_U32 },
	[KTEST_A_SNAM]  = { .type = NLA_STRING },
	[KTEST_A_TNAM]  = { .type = NLA_STRING },
	[KTEST_A_NUM]   = { .type = NLA_U32 },
	[KTEST_A_STAT]  = { .type = NLA_U32 },
	[KTEST_A_HID]   = { .type = NLA_U32 },
	[KTEST_A_LIST]  = { .type = NLA_NESTED },
	[KTEST_A_TEST]  = { .type = NLA_NESTED },
	[KTEST_A_HLIST] = { .type = NLA_NESTED },
	[KTEST_A_STR]   = { .type = NLA_STRING },
	[KTEST_A_FILE]  = { .type = NLA_STRING },
};
#endif

/* supported commands */
enum ktest_cmd {
	KTEST_C_UNSPEC,
	KTEST_C_REQ,
	KTEST_C_RESP,
	KTEST_C_MAX
};


#ifdef __cplusplus
}
#endif
#endif
