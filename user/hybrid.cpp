// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * hybrid.cpp: User mode part of the hybrid_self
 * test in selftests
 */

#include "ktf.h"
#include <string.h>

extern "C" {
#include "../selftest/hybrid_self.h"
}

/* User side of a simple hybrid test that just sends an out-of-band message
 * to the kernel side - the kernel implementation picks it up and verifies
 * that it is the expected string and integer values.
 *
 * This form of test allows the mixing of normal gtest user land assertions
 * with one or more calls to the kernel side to run tests there:
 */

HTEST(selftest, msg)
{
  KTF_USERDATA(self, hybrid_self_params, data);

  strcpy(data->text_val, HYBRID_MSG);
  data->val = HYBRID_MSG_VAL;

  /* assertions can be specified here: */
  EXPECT_TRUE(true);

  ktf::run(self);

  /* and here.. */
  EXPECT_TRUE(true);
}
