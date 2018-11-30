/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * ktf.h: User mode side of KTF extensions to the gtest unit test framework.
 * Include this to write hybrid tests
 *
 */
#ifndef _KTF_H
#define _KTF_H
#include <gtest/gtest.h>

namespace ktf
{

  /* Interfaces intended to be used directly by programs:
   * ----------------------------------------------------
   */
  class KernelTest;

  /* Invoke the kernel test - to be called directly from user mode
   * hybrid tests:
   */
  void run(KernelTest* kt, std::string ctx = "");

  /* Function for enabling/disabling coverage for module */
  int set_coverage(std::string module, unsigned int opts, bool enabled);

} // end namespace ktf

/* HTEST: Define user part of a hybrid test.
 * Hybrid tests are tests that have a user and a kernel counterpart,
 * to allow testing of interaction between user mode and the kernel:
 */
#define HTEST(__setname,__testname)	\
  class __setname ## _ ## __testname : public ktf::test_cb	\
  {\
  public:\
    __setname ## _ ## __testname() {\
      ktf::add_wrapper(#__setname,#__testname,as_test_cb()); \
    }\
    virtual void fun(ktf::KernelTest* kt);	\
  }; \
  __setname ## _ ## __testname \
     __setname ## _ ## __testname ## _value;\
  void __setname ## _ ## __testname::fun(ktf::KernelTest* self)


/* Part of KTF support for hybrid tests: allocate/get a reference to
 * an out-of-band user data pointer:
 */
#define KTF_USERDATA(__kt_ptr, __priv_datatype, __priv_data) \
  struct __priv_datatype *__priv_data =	\
    (struct __priv_datatype *)get_priv(__kt_ptr, sizeof(struct __priv_datatype)); \
  ASSERT_TRUE(__priv_data); \
  ASSERT_EQ(get_priv_sz(__kt_ptr), sizeof(struct __priv_datatype))


/* Private interfaces (needed by definition of HTEST)
 * --------------------------------------------------
 */

namespace ktf {
  class test_cb
  {
  public:
    virtual ~test_cb() {}
    virtual test_cb* as_test_cb() { return this; }
    virtual void fun(KernelTest* kt) {}
  };

  /* Function for adding a user level test wrapper */
  void add_wrapper(const std::string setname, const std::string testname,
		   test_cb* tcb);

  /* get a priv pointer of the given size, allocate if necessary */
  void* get_priv(KernelTest* kt, size_t priv_sz);

  /* Get the size of the existing priv data */
  size_t get_priv_sz(KernelTest *kt);

  // Initialize KTF - should normally be called as part of static
  // initialization, but some compilers may decide to optimize it away:
  int setup(void);
} // end namespace ktf

#endif
