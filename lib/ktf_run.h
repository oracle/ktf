/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * ktf_run.h: googletest integration of KTF - interface towards googletest
 */

#ifndef KTF_RUN_H
#define KTF_RUN_H
#include "utest.h"
#include <gtest/gtest.h>

namespace utest
{
  void gtest_handle_test(int result,  const char* file, int line, const char* report);
}

#endif
