// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_run.cpp:
 *  Gtest integration of ktf kernel tests -
 *  e.g. tests that are fully implemented on the test driver side
 *  and only initiated via run_test below
 */

#include "ktf_int.h"
#include <assert.h>
#include <errno.h>
#include "ktf_debug.h"

namespace ktf
{

class KernelMetaFactory;

class Kernel : public ::testing::TestWithParam<std::string>
{
public:
  Kernel()
  {
    assert(false); // Should not be hit but is needed for template resolving
  }

  Kernel(std::string& setname, std::string& testname)
  {
    log(KTF_INFO, "%s.%s\n", setname.c_str(), testname.c_str());

    ukt = ktf::find_test(setname,testname,&ctx);
    if (!ukt) {
      fprintf(stderr, "**** Internal error: Could not find test %s.%s (set %s, name %s) ****\n",
	      setname.c_str(), testname.c_str(), setname.c_str(), testname.c_str());
      exit(7);
    }
    log(KTF_INFO, "### Kernel ctor %s (%ld,%ld)\n", ukt->name.c_str(), ukt->setnum, ukt->testnum);
  }

  virtual ~Kernel()
  {
    log(KTF_INFO, "### Kernel dtor %s\n", ukt->name.c_str());

    /* For some reason errno sometimes get set
     * TBD: Figure out why - for now just reset it to avoid confusing the next test!
     */
    if (errno) {
      log(KTF_INFO, "### %s: errno was set to %d - resetting..\n", ukt->name.c_str(), errno);
      errno = 0;
    }
  }

  virtual void TestBody();
private:
  ktf::KernelTest* ukt;
  std::string ctx;
  friend void setup(configurator c);
  static int AddToRegistry();
  static configurator configurator_;
};



class TFactory : public ::testing::internal::ParameterizedTestFactory<Kernel>
{
public:
  TFactory(std::string s, ParamType parameter)
    : ::testing::internal::ParameterizedTestFactory<Kernel>(parameter),
      setname(s)
  {
    testname = parameter.c_str();
  }

  virtual ::testing::Test* CreateTest()
  {
    return new Kernel(setname,testname);
  }

private:
  std::string setname;
  std::string testname;
};


class KernelMetaFactory : public ::testing::internal::TestMetaFactory<Kernel>
{
public:
  virtual ::testing::internal::TestFactoryBase* CreateTestFactory(ParamType parameter) {
    TFactory* tf;
    std::string setname = get_current_setname();
    tf = new TFactory(setname, parameter.c_str());
    return tf;
  }
};

testing::internal::ParamGenerator<Kernel::ParamType> gtest_query_tests(void);
std::string gtest_name_from_info(const testing::TestParamInfo<Kernel::ParamType>&);
void gtest_handle_test(int result,  const char* file, int line, const char* report);

#ifndef INSTANTIATE_TEST_SUITE_P
/* This rename happens in Googletest commit 3a460a26b7.
 * Make sure we compile both before and after it:
 */
#define AddTestSuiteInstantiation AddTestCaseInstantiation
#endif

int Kernel::AddToRegistry()
{
  if (!ktf::setup(ktf::gtest_handle_test)) return 1;

  /* Run query against kernel to figure out which tests that exists: */
  stringvec& t = ktf::query_testsets();

  ::testing::internal::ParameterizedTestCaseInfo<Kernel>* tci =
      ::testing::UnitTest::GetInstance()->parameterized_test_registry()
      .GetTestCasePatternHolder<Kernel>( "Kernel", ::testing::internal::CodeLocation("", 0));

  for (stringvec::iterator it = t.begin(); it != t.end(); ++it)
  {
    ::testing::internal::TestMetaFactory<Kernel>* mf = new KernelMetaFactory();
#if HAVE_CODELOC_FOR_ADDTESTPATTERN
    tci->AddTestPattern(it->c_str(), "", mf, ::testing::internal::CodeLocation("", 0));
#else
    tci->AddTestPattern(it->c_str(), "", mf);
#endif
  }

  tci->AddTestSuiteInstantiation("", &gtest_query_tests, &gtest_name_from_info, NULL, 0);
  return 0;
}

void setup(configurator c)
{
  ktf::set_configurator(c);
  Kernel::AddToRegistry();
}


void Kernel::TestBody()
{
  run_test(ukt, ctx);
}


void gtest_handle_test(int result,  const char* file, int line, const char* report)
{
  if (result >= 0) {
    const ::testing::AssertionResult gtest_ar =
      !result ? (testing::AssertionFailure() << report) : testing::AssertionSuccess();

    if (result) {
      /* We might get multiple partial results from the kernel in one positive
       * result report:
       */
#if HAVE_ASSERT_COUNT
      ::testing::UnitTest::GetInstance()->increment_success_assert_count(result);
#else
      GTEST_SUCCEED();
#endif
    } else {
      ::testing::internal::AssertHelper(::testing::TestPartResult::kNonFatalFailure,
					file, line, gtest_ar.failure_message()) = ::testing::Message();
    }
  }
}

testing::internal::ParamGenerator<Kernel::ParamType> gtest_query_tests()
{
  return testing::ValuesIn(ktf::get_test_names());
}

std::string gtest_name_from_info(const testing::TestParamInfo<Kernel::ParamType>& info)
{
  return info.param;
}

} // end namespace ktf
