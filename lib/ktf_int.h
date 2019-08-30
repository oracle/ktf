// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_int.h: User mode side of extension to the gtest unit test framework:
 *  1) Kernel test support via netlink
 *  2) Standard command line parameters
 *
 * This file exposes some internals - for users of hybrid tests including
 * ktf.h should be sufficient:
 */

#ifndef KTF_INT_H
#define KTF_INT_H
#include <string>
#include <vector>
#include "ktf.h"

typedef std::vector<std::string> stringvec;

namespace ktf
{

  /* A callback handler to be called for each assertion result */
  typedef void (*test_handler)(int result,  const char* file, int line, const char* report);

  class KernelTest
  {
  public:
    KernelTest(const std::string& setname, const char* testname, unsigned int handle_id);
    ~KernelTest();
    void* get_priv(size_t priv_sz);
    size_t get_priv_sz(KernelTest *kt);
    std::string setname;
    std::string testname;
    unsigned int handle_id;
    std::string name;
    size_t setnum;  /* This test belongs to this set in the kernel */
    size_t testnum; /* This test's index (test number) in the kernel */
    void* user_priv;  /* Optional private data for the test */
    size_t user_priv_sz; /* Size of the user_priv data if used */
    test_cb* user_test;  /* Optional user level wrapper function for the kernel test */
    char* file;
    int line;
  };

  void *get_priv(KernelTest *kt, size_t priv_sz);

  // Set up connection to the kernel test driver:
  // @handle_test contains the test framework's handling code for test assertions */
  bool setup(test_handler handle_test);

  void set_configurator(configurator c);

  // Parse command line args (call after gtest arg parsing)
  char** parse_opts(int argc, char** argv);

  /* Query kernel for available tests in index order */
  stringvec& query_testsets();

  stringvec get_testsets();
  std::string get_current_setname();
  stringvec get_test_names();

  /* "private" - only run from gtest framework */
  void run_test(KernelTest* test, std::string& ctx);
} // end namespace ktf


/* Redefine for C++ until we can get it patched - type mismatch by default */
#ifdef nla_for_each_nested
#undef nla_for_each_nested
#endif
#define nla_for_each_nested(pos, nla, rem) \
  for (pos = (struct nlattr*)nla_data(nla), rem = nla_len(nla);	\
             nla_ok(pos, rem); \
             pos = nla_next(pos, &(rem)))

#endif
