/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * ktf_unlproto.h: implements interfaces for user-kernel netlink interactions
 *   for querying/running tests.
 */
#ifndef _KTF_UNLPROTO_H
#define _KTF_UNLPROTO_H
#ifdef __cplusplus
extern "C" {
#endif

enum ktf_cmd_type {
	KTF_CT_UNSPEC,
	KTF_CT_QUERY,
	KTF_CT_RUN,
	KTF_CT_COV_ENABLE,
	KTF_CT_COV_DISABLE,
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
	KTF_A_MOD,    /* module for coverage analysis */
	KTF_A_COVOPT, /* options for coverage analysis */
	KTF_A_DATA,   /* Binary data used by a.o. hybrid tests */
	KTF_A_MAX
};

/* attribute policy */
#ifdef NL_INTERNAL
static struct nla_policy ktf_gnl_policy[KTF_A_MAX] = {
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
	[KTF_A_MOD]   = { .type = NLA_STRING },
	[KTF_A_COVOPT] = { .type = NLA_U32 },
	[KTF_A_DATA] = { .type = NLA_BINARY },
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
	KTF_VSHIFT_MICRO = 16,
	KTF_VSHIFT_MINOR = 32,
	KTF_VSHIFT_MAJOR = 48
};

#define KTF_VERSION(__field, __v) \
	((__v & (0xffffULL << KTF_VSHIFT_##__field)) \
	>> KTF_VSHIFT_##__field)

#define KTF_VERSION_SET(__field, __v) \
	((__v & 0xffffULL) << KTF_VSHIFT_##__field)

#define	KTF_VERSION_LATEST	\
	(KTF_VERSION_SET(MAJOR, 0ULL) | KTF_VERSION_SET(MINOR, 1ULL))

/* Coverage options */
#define	KTF_COV_OPT_MEM		0x1

#ifdef __cplusplus
}
#endif
#endif
