/* Copyright (c) 2011 Oracle Corporation. All rights reserved.
 *
 * Oracle SIF Infiniband PCI Express host channel adapter (HCA)
 *   device driver for Linux
 *
 * ktest_run.h: Gtest integration of kernel tests
 *
 */

#ifndef SIF_KTEST_RUN_H
#define SIF_KTEST_RUN_H
#include "utest.h"
#include <gtest/gtest.h>

namespace utest
{
  void gtest_handle_test(int result,  const char* file, int line, const char* report);

  void gtest_init();
}

#endif

