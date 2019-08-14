// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktfrun.cpp: Generic user level application to run kernel tests
 *   provided by modules subscribing to ktf services.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ktf.h>

extern "C" {
#include "../selftest/context_self.h"
}

void selftest_configure()
{
  struct test_parameter_block p;
  memset(&p, 0, sizeof(p));
  strcpy(p.s, CONTEXT_MSG);

  /* First configure two contexts provided by the kernel part: */
  p.magic = CONTEXT_MAGIC1;
  KTF_CONTEXT_CFG("context1", "context_type_1", test_parameter_block, &p);
  p.magic = CONTEXT_MAGIC2;
  KTF_CONTEXT_CFG("context2", "context_type_2", test_parameter_block, &p);

  /* Configure a 3rd, dynamically created context, using CONTEXT3_TYPE_ID
   * which the kernel part has enabled for dynamic creation of contexts
   * from user space (see kernel/context.c: add_context_tests()
   * for details of setup)
   */
  p.magic = CONTEXT_MAGIC3;
  KTF_CONTEXT_CFG("context3", "context_type_3", test_parameter_block, &p);
}


int main (int argc, char** argv)
{
  ktf::setup(selftest_configure);
  testing::InitGoogleTest(&argc,argv);

  return RUN_ALL_TESTS();
}
