// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * hybrid_self.h: The data structure passed between user level and kernel for the
 *  hybrid self tests. Included both from user space and kernel space and
 *  needs to be a C struct.
 */

#ifndef KTF_HYBRID_SELF_H
#define KTF_HYBRID_SELF_H

#define HYBRID_SELF_MAX_TEXT 127

struct hybrid_self_params
{
	char text_val[HYBRID_SELF_MAX_TEXT+1];
	unsigned long val;
};


/* Constants for the selftest.msg test: */
#define HYBRID_MSG "a little test string"
#define HYBRID_MSG_VAL  0xffUL

#endif
