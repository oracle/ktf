# Kernel Test Framework (KTF)

KTF is a Google Test-like environment for writing C unit tests for
kernel code.  Tests are implemented as kernel modules which declare
each test as part of a test case.  The body of each test case
consists of assertions.  Tests look like this:

	TEST(examples, hello_ok)
	{
		EXPECT_TRUE(true);
	}

"examples" is the test case name, "hello_ok" the test.
KTF provides many different types of assertions, see
kernel/ktf.h for the complete list.

Usually tests are added on test module init via

	ADD_TEST(test_name);

This registers the test with the KTF framework for later
execution.  There are many examples in the examples/
directory.

"ktfrun" is provided to execute tests, it communicates
with the KTF kernel module via netlink socket to query
available tests and trigger test execution.

The design priorities for KTF are to make it

 * easy to run tests.  Just ensure the ktf module is loaded,
   then load your test module and execute "ktfrun".

 * easy to interpret results.  Output from ktfrun is clear
   and can be filtered easily.  Assertion failures indicate
   the line of code where the failure occurred.  Results of
   the last test run are always available from
   /sys/kernel/debug/ktf/results/<test case name>

 * easy to add tests.  Adding a test takes a few lines of code.
   Just (re)build the test module, unload/reload and KTF can
   run the test.  See the examples/ directory for some hints.

 * easy to analyse test behaviour (code coverage, memory utilization
   during test execution).  We provide "ktfcov" to support enabling
   coverage support on a per-module basis.  Coverage data is
   available in /sys/kernel/debug/ktf/coverage, showing how often
   functions were called during the coverage period, and optionally
   any outstanding memory allocations originating from functions
   that were subject to coverage.

All of the above will hopefully help Linux kernel engineers
practice continuous integration and more thoroughly unit test
their code.

## User Documentation

See [./doc](./doc)


