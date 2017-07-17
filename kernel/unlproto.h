#ifndef _KTF_UNLPROTO_H
#define _KTF_UNLPROTO_H
#ifdef __cplusplus
extern "C" {
#endif

enum ktf_cmd_type {
	KTF_CT_UNSPEC,
	KTF_CT_QUERY,
	KTF_CT_RUN,
	KTF_CT_MAX,
};

/* Netlink protocol definition shared between user and kernel space
 * Include once per user app as it defines struct values!
 */

/* supported attributes */
enum ktf_attr {
	KTF_A_UNSPEC,
	KTF_A_TYPE,
	KTF_A_VERSION,
	KTF_A_SNAM,
	KTF_A_TNAM,
	KTF_A_NUM,
	KTF_A_STR,
	KTF_A_FILE,
	KTF_A_STAT,
	KTF_A_LIST,
	KTF_A_TEST,
	KTF_A_HID,    /* Test handle ID */
	KTF_A_HLIST,  /* List of handles repr. as a LIST of contexts for a given HID */
	KTF_A_MAX
};

/* attribute policy */
#ifdef NL_INTERNAL
struct nla_policy ktf_gnl_policy[KTF_A_MAX] = {
	[KTF_A_TYPE]  = { .type = NLA_U32 },
	[KTF_A_VERSION] = { .type = NLA_U64 },
	[KTF_A_SNAM]  = { .type = NLA_STRING },
	[KTF_A_TNAM]  = { .type = NLA_STRING },
	[KTF_A_NUM]   = { .type = NLA_U32 },
	[KTF_A_STAT]  = { .type = NLA_U32 },
	[KTF_A_HID]   = { .type = NLA_U32 },
	[KTF_A_LIST]  = { .type = NLA_NESTED },
	[KTF_A_TEST]  = { .type = NLA_NESTED },
	[KTF_A_HLIST] = { .type = NLA_NESTED },
	[KTF_A_STR]   = { .type = NLA_STRING },
	[KTF_A_FILE]  = { .type = NLA_STRING },
};
#endif

/* supported commands */
enum ktf_cmd {
	KTF_C_UNSPEC,
	KTF_C_REQ,
	KTF_C_RESP,
	KTF_C_MAX
};

enum ktf_vshift {
	KTF_VSHIFT_BUILD = 0,
	KTF_VSHIFT_MICRO = 8,
	KTF_VSHIFT_MINOR = 16,
	KTF_VSHIFT_MAJOR = 24
};

#define KTF_VERSION(__field, __v) \
	((unsigned long long)((__v & (0xffff << KTF_VSHIFT_##__field)) \
	>> KTF_VSHIFT_##__field))

#define KTF_VERSION_SET(__field, __v) \
	((unsigned long long)(__v << KTF_VSHIFT_##__field))

#define	KTF_VERSION_LATEST	\
	(KTF_VERSION_SET(MAJOR, 0) | KTF_VERSION_SET(MINOR, 1))

#ifdef __cplusplus
}
#endif
#endif
