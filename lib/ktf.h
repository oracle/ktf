// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
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

  typedef void (*configurator)(void);

  // Initialize KTF:
  // If necessary, supply a callback that uses the KTF_CONTEXT_CFG* macros below
  // to configure any necessary contexts:
  void setup(configurator c = NULL);

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

/* KTF support for configurable contexts:
 * Send a configuation data structure to the given context name.
 */
#define KTF_CONTEXT_CFG(__context_name, __context_type_name, __priv_datatype, __priv_data) \
  ktf::configure_context(__context_name, __context_type_name, \
  			 (struct __priv_datatype *)__priv_data, \
			 sizeof(__priv_datatype))
/* Alternative to KTF_CONTEXT_CFG: If there are multiple contexts with the same name
 * (but with different handles) use a test name to identify the context to be configured
 */
#define KTF_CONTEXT_CFG_FOR_TEST(__test_name, __context_type_name, __priv_datatype, __priv_data) \
  ktf::configure_context_for_test(__test_name, __context_type_name, \
				  (struct __priv_datatype *)__priv_data, \
				  sizeof(__priv_datatype))

#define KTF_FIND(__setname, __testname, __context) \
  ktf::find_test(__setname, __testname, __context)

/* Private interfaces (needed by macro definitions above)
 * ------------------------------------------------------
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

  // Configure ktf context - to be used via KTF_CONTEXT_CFG*():
  void configure_context(const std::string context, const std::string type_name,
			 void *data, size_t data_sz);
  void configure_context_for_test(const std::string testname, const std::string type_name,
				  void *data, size_t data_sz);

  KernelTest* find_test(const std::string& setname, const std::string& testname,
			std::string* ctx);

} // end namespace ktf

#endif
