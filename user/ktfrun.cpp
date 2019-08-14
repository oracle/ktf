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

int main (int argc, char** argv)
{
  ktf::setup();
  testing::InitGoogleTest(&argc,argv);

  return RUN_ALL_TESTS();
}
