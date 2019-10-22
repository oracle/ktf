// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_unlproto.h: implements interfaces for user-kernel netlink interactions
 *   for querying/running tests.
 */
#ifndef _KTF_UNLPROTO_H
#define _KTF_UNLPROTO_H
#ifdef __cplusplus
extern "C" {
#endif

/* supported commands */
enum ktf_cmd {
	KTF_C_UNSPEC,
	KTF_C_QUERY,	/* Query for a list of tests */
	KTF_C_RUN,	/* Run a test */
	KTF_C_COV,	/* Enable/disable coverage support */
	KTF_C_CTX_CFG,	/* Configure a context */
	KTF_C_MAX,
};

/* Netlink protocol definition shared between user and kernel space
 * Include once per user app as it defines struct values!

 * Each of the above commands issues from user space are responded to by the kernel
 * with the same command ID, but the kernel and user side messages have
 * different structure. See BNF syntax below for details.
 *
 * QUERY:
 * ------
 * The QUERY request message is simply a VERSION attribute encoded according to the KTF_VERSION
 * macro below. All messages require this version number as a sanity check.
 *
 * The QUERY response from the kernel contains a list (HLIST attribute) of
 *   - available handles (handle IDs(HID), each with an optional list (LIST attribute)
 *     of associated contexts with context names in STR and and an optional context type
 *     with name in MOD, and the status of the context (unconfigured, configured ok, or errno)
 *     in the STAT attribute.
 *   - all existing test suites/sets tests and their names as a list using LIST with a nested
 *     list of TEST lists, each representing a test suite and corresponding tests and associated
 *     test handle:
 *
 * <QUERY_request>   ::= VERSION
 *
 * <QUERY_response>  ::= VERSION [ <handle_list> ] NUM [ <testset_list> ]
 * <handle_list>     ::= HLIST <handle_data>+
 * <handle_data>     ::= HID [ <context_list> ]
 * <context_list>    ::= LIST <context_type>+ <context_data>+
 * <context_type>    ::= FILE
 * <context_data>    ::= STR [ MOD ] STAT
 * <testset_list>    ::= LIST <testset_data>+
 * <testset_data>    ::= STR TEST <test_data>+
 * <test_data>       ::= HID STR
 *
 *
 * RUN:
 * ----
 * A RUN request currently specifies a run of a single named test. A test is identified
 * by a test SNAME (set/suite name) a TNAM (test name) and an optional context (STR attribute)
 * to run it in. In addition tests can be arbitrarily parameterized, so tests optionally
 * allow out-of-band data via a DATA binary attribute.
 * The kernel response is a global status (in STAT) pluss an optional set of test results
 * eg. it supports running multiple tests, but this is not currently used.
 *
 * Each test result contains an optional list of individual error reports and
 * which each contains file name (FILE), line number (NUM) and a formatted error report string.
 * In addition each test result reports the number of assertions that were executed in the STAT
 * attribute:
 *
 * <RUN_request>     ::= VERSION SNAM TNAM [ STR ][ DATA ]
 * <RUN_response>    ::= STAT LIST <test_result>
 * <test_run_result> ::= STAT [ LIST <error_report>+ ]
 * <error_report>    ::= STAT FILE NUM STR
 *
 * COV:
 * ----
 * A COV request is currently used to either enable or disable (NUM = 1/0)
 * coverage support for a particular module given by MOD, with option
 * flags (COVOPT):
 *
 * <COV_request>     ::= VERSION MOD NUM [ COVOPT ]
 * <COV_response>    ::= NUM STAT
 *
 * CTX_CFG:
 * --------
 * A context configuration (CTX_CFG) request is used to configure the kernel side
 * of a context with the necessary parameters (context type specific data) provided
 * in a DATA attribute. The optional context type parameter (FILE attribute)
 * can be used to reference a context type, to dynamically create a new context
 * if the name given as STR does not exist.
 * The kernel currently does not send any response data to the user,
 * but tests will obviously subsequently fail if the context is not properly
 * configured:
 *
 * <CTX_CFG_request> ::= VERSION STR HID DATA [ FILE ]
 *
 */

/* supported attributes */
enum ktf_attr {
	KTF_A_UNSPEC,
	KTF_A_VERSION,/* KTF version */
	KTF_A_SNAM,   /* Test suite name */
	KTF_A_TNAM,   /* Test name */
	KTF_A_NUM,
	KTF_A_STR,
	KTF_A_FILE,
	KTF_A_STAT,
	KTF_A_LIST,
	KTF_A_TEST,
	KTF_A_HID,    /* Test handle ID */
	KTF_A_HLIST,  /* List of handles repr. as a LIST of contexts for a given HID */
	KTF_A_MOD,    /* module for coverage analysis, also used for context type */
	KTF_A_COVOPT, /* options for coverage analysis */
	KTF_A_DATA,   /* Binary data used by a.o. hybrid tests */
	KTF_A_MAX
};

/* attribute policy */
#ifdef NL_INTERNAL
static struct nla_policy ktf_gnl_policy[KTF_A_MAX] = {
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
	(KTF_VERSION_SET(MAJOR, 0ULL) | KTF_VERSION_SET(MINOR, 2ULL) | KTF_VERSION_SET(MICRO, 1ULL))

/* Coverage options */
#define	KTF_COV_OPT_MEM		0x1

struct nla_policy *ktf_get_gnl_policy(void);

#ifdef __cplusplus
}
#endif
#endif
