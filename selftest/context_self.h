// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * context_self.h: The data structure passed between user level and kernel for the
 *  hybrid self tests. Included both from user space and kernel space and
 *  needs to be a C struct.
 */

#ifndef KTF_CONTEXT_SELF_H
#define KTF_CONTEXT_SELF_H

#define CONTEXT_SELF_MAX_TEXT 30

/* A simple example parameter block:
 * For verification purposes it can be useful to have a field
 * like 'magic' below, which serves for the purpose of
 * a sanity check that the parameters sent by the user program
 * actually corresponds to what the kernel expects:
 */
struct test_parameter_block {
	long magic;
	long myvalue;
	char s[CONTEXT_SELF_MAX_TEXT+1];
};

/* Constants for the selftest.param_context test: */
#define CONTEXT_MSG "from user to kernel"
#define CONTEXT_MAGIC1 0xfaaa1234UL
#define CONTEXT_MAGIC2 0xaabbccUL
#define CONTEXT_MAGIC3 0x123456UL

#endif
