/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * ktfrun.cpp: Generic user level application to run kernel tests
 *   provided by modules subscribing to ktf services.
 */
#include <stdio.h>
#include <stdlib.h>
#include "lib/ktf_run.h"
#include "lib/debug.h"

int main (int argc, char** argv)
{
  testing::GTEST_FLAG(output) = "xml:ktfrun.xml";
  testing::InitGoogleTest(&argc,argv);

  return RUN_ALL_TESTS();
}
