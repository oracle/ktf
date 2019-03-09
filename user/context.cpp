/* Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * context.cpp: User mode configuration of a
 * kernel side context for selftests
 */

#include "ktf.h"
#include <string.h>

extern "C" {
#include "../selftest/context_self.h"
}


class SelftestEnv : public testing::Environment
{
public:
  SelftestEnv()
  {
    AddGlobalTestEnvironment(this);
  }

  virtual ~SelftestEnv() {}

  virtual void SetUp()
  {
    struct test_parameter_block p;
    memset(&p, 0, sizeof(p));
    strcpy(p.s, CONTEXT_MSG);
    p.magic = CONTEXT_MAGIC1;
    KTF_CONTEXT_CFG("context1", CONTEXT1_TYPE_ID, test_parameter_block, &p);
    p.magic = CONTEXT_MAGIC2;
    KTF_CONTEXT_CFG("context2", CONTEXT2_TYPE_ID, test_parameter_block, &p);
  }
};


SelftestEnv* foo = new SelftestEnv();
