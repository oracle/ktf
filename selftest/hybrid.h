// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * hybrid.h: Hybrid (combined user level and kernel) self tests,
 *  kernel side, internal interface:
 */

#ifndef KTF_HYBRID_H
#define KTF_HYBRID_H

#include "hybrid_self.h"

/* The kernel part of hybrid tests must be added to KTFs set of tests like any other tests,
 * in fact from KTF's kernel perspective it is like any other test, except that it likely will
 * fail if called without the context provided from the user space side.
 *
 * This function adds the tests declared in hybrid.c
 */
void add_hybrid_tests(void);


#endif
