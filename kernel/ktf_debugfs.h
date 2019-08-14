// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Alan Maguire <alan.maguire@oracle.com>
 *
 * ktf_debugfs.h: Support for creating a debugfs representation of test
 * sets/tests.
 */

#ifndef KTF_DEBUGFS_H
#define KTF_DEBUGFS_H
#include <linux/debugfs.h>

#define KTF_DEBUGFS_ROOT                        "ktf"
#define KTF_DEBUGFS_RUN                         "run"
#define KTF_DEBUGFS_RESULTS                     "results"
#define KTF_DEBUGFS_COV				"coverage"
#define KTF_DEBUGFS_TESTS_SUFFIX                "-tests"

#define KTF_DEBUGFS_NAMESZ                      256

struct ktf_test;
struct ktf_case;

void ktf_debugfs_create_test(struct ktf_test *);
void ktf_debugfs_destroy_test(struct ktf_test *);
void ktf_debugfs_create_testset(struct ktf_case *);
void ktf_debugfs_destroy_testset(struct ktf_case *);
void ktf_debugfs_init(void);
void ktf_debugfs_cleanup(void);


#endif
