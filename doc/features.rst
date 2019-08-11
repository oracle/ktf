3. KTF kernel specific features
-------------------------------

Reference to module internal symbols
************************************

When working with unit tests, the need to access non-public interfaces
often arises. In general non-public interfaces is of course not intended to
be used by outside programs, but a test framework is somewhat special here
in that it is often necessary or desirable to unit test internal
data structures or algorithms even if they are not exposed. The program
under test may be a complicated beast by itself and merely exercising the
public interfaces may not be flexible enough to stress the internal code.
Even if it is possible to get the necessary "pressure" from the outside
like that, it might be much more challenging or require a lot more work.

The usual method to gain access to internal interfaces is to be part of the
internals. To some extent this is the way a lot of the kernel testing
utilities operate. The obvious advantages of this is that the test code
'automatically' follows the module and it's changes. The disadvantage is
that test code is tightly integrated with the code itself. One important
goal with KTF is to make it possible to write detailed and sophisticated
test code which does not affect the readability or complexity of the tested
code.

KTF contains a small python program, ``resolve``, which
parses a list of symbol names on the form::

    #module first_module
    #header first_header.h
    private_symbol1
    private_symbol2
    ...
    #header second_header.h
    #module next_module
    ...

The output is a header file and a struct containing function pointers and
some convenience macro definitions to make it possible to 'use' the
internal functions just as one would if within the module. This logic is
based on kallsyms, and would of course only work if that functionality is
enabled in the kernel KTF compiles against. Access to internal symbols
this way is controlled by the kernel config options CONFIG_KALLSYMS
and CONFIG_KALLSYMS_ALL, which must be set to "y".

If you create a new test project using the ``ktfnew`` script, you can
put your private symbol definitions in a file ``ktf_syms.txt`` in the
kernel directory, and KTF will automatically generate ``ktf_syms.h``,
which you can then include in your test file to get to these symbols.
This functionality is also used by the KTF selftests, which might
serve as an example to get started.

Note also that for exported symbols, if you build your module out-of-tree in
addition to KTF and the test modules, you might need to also add those
module's ``Module.symvers`` files to ``KBUILD_EXTRA_SYMBOLS``
(See kernel documentation for this) to find them during test module build.

Requesting callbacks when a certain function gets called/returns
****************************************************************

Tap into function entry using KTF entry probes.  Many tests need to
move beyond kernel APIs and ensure that side effects (logging a
message etc) occur.  A good way to do this is to probe entry of relevant
functions.  In order to do so in KTF you need to:

    - define an entry probe function with the same return value and arguments
      as the function to be probed

    - within the body of the entry probe function, ensure return is wrapped with
      KTF_ENTRY_PROBE_RETURN(<return value>);

    - entry probes need to registered for use and de-registered when done via
      KTF_[UN]REGISTER_ENTRY_PROBE(<kernel function name>, <handler function>).

See example h4.c in examples/ for a simple case where we probe printk() and
ensure it is called.

Sometimes is is also useful to check that an intermediate function is returning
an expected value.  Return probes can be used to register/probe function
return.  In order to probe function return:

    - define a return probe point; i.e
      KTF_RETURN_PROBE(<kernel function>, <handler>)

    - within the body of the return probe the return value can be retrieved
      via KTF_RETURN_VALUE().  Type will obviously depend on the function
      probed so should be cast if dereferencing is required.

    - return probes need to be registered for use and unregistered when done
      via KTF_[UN]REGISTER_RETURN_PROBE(<kernel function name>, <handler>).

See example h4.c in examples/ for a simple case where we verify return value
of printk().

Note that this functionality is only available on kernels with CONFIG_KPPROBES
and CONFIG_KRETPROBES set to "y".

Overriding functions
********************
in some cases, we wish to override harmful functions when inducing failues in
tests (e.g. skb_panic()). Override is done via kprobes and we define as follows::

    KTF_OVERRIDE(oldfunc, newfunc)
    {
	...
	KTF_SET_RETURN_VALUE(1);
	KTF_OVERRIDE_RETURN;
    }

    TEST(...)
    {
	KTF_REGISTER_OVERRIDE(oldfunc, newfunc);
	...
	KTF_UNREGISTER_OVERRIDE(oldfunc, newfunc);
    }

Override should be used sparingly; we'd rather test the code as-is and use
entry/return probes where possible.

Note that this functionality is only available on kernels with CONFIG_KPPROBES
and CONFIG_KRETPROBES set to "y".

Coverage analytics
******************

While other coverage tools exist, they generally involve gcc-level support
which is required at compile-time.  KTF offers kernel module coverage
support via kprobes instead.  Tests can enable/disable coverage on a
per-module basis, and coverage data can be retrieved via::

    # more /sys/kernel/debug/ktf/coverage

For a given module we show how many of its functions were called versus the
total, e.g.::

    # cat /sys/kernel/debug/ktf/coverage
    MODULE               #FUNCTIONS    #CALLED
    selftest             14            1

We see 1 out of 14 functions was called when coverage was enabled.

We can also see how many times each function was called::

    MODULE          FUNCTION                   COUNT
    selftest        myelem_free                0
    selftest        myelem_cmp                 0
    selftest        ktf_return_printk          0
    selftest        cov_counted                1
    selftest        dummy                      0

In addition, we can track memory allocated via kmem_cache_alloc()/kmalloc()
originating from module functions we have enabled coverage for.  This
allows us to track memory associated with the module specifically to find
leaks etc.  If memory tracking is enabled, /sys/kernel/debug/ktf/coverage
will show outstanding allocations - the stack at allocation time; the
memory address and size.

Coverage can be enabled via the "ktfcov" utility.  Syntax is as follows::

    ktfcov [-d module] [-e module [-m]]

"-e" enables coverage for the specified module; "-d" disables coverage.
"-m" in combination with "-e" enables memory tracking for the module under
test.

Note that this functionality is only available on kernels with CONFIG_KPPROBES
and CONFIG_KRETPROBES set to "y", and that CONFIG_KALLSYMS and
CONFIG_KALLSYMS_ALL should be set to "y" also to get all exported and
non-exported symbols.

Thread execution
****************

KTF provides easy mechanisms to create and use kernel threads.
Assertions can then be carried out in the created thread context
also.  Threads can be created as follows, and we can if we wish
wait for thread completion::


    TEST(foo, bar)
    {
        struct ktf_thread t;

        ...
        KTF_THREAD_INIT(mythread, &t);
        KTF_THREAD_RUN(&t);
        KTF_THREAD_WAIT_COMPLETED(&t);
        ...
    }

The thread itself is defined as follows::

    KTF_THREAD(mythread)
    {
        ...
    }

We can add assertions to the thread and they will be recorded/logged
as part of the test.

Hybrid tests
************

KTF also allows mixing of user and kernel side code in the same test.
This is useful if one wants for instance to verify that user land operations
has certain effects in the kernel, for instance verify that a parameter is
transferred or handled correctly in the kernel.

Hybrid tests are specified by writing a user mode test using the special
``HTEST()`` macro instead of the normal ``TEST()`` macro. This macro takes
Inside the macro, the special variable ``self`` can be used to refer to the
test itself, and the macro ``KTF_USERDATA()`` can be used to get a pointer to
an allocated instance of a test specific parameter struct. The user land test
can then call the kernel side directly using ``ktf::run_kernel_test(self)`` An
optional context name can be specified as a second argument to the call if
needed. This can be done any number of times during the user land test and
each call will transmit the struct value out-of-band to the kernel side. To
the kernel this appears as separate test calls, but the kernel side have the
option of aggregating or otherwise maintain state for the duration of the
test.

Declare the data structure to use for user/kernel out-of-band communication
in a header file that is included both by the user and the kernel side::

    struct my_params
    {
	char expected[128];
	unsigned long mode;
    };

The user land side of the test itself can then look like this::

    HTEST(foo, hybrid)
    {
	KTF_USERDATA(self, my_params, data);

	<normal gtest code>

	strcpy(data->expected, "something");
	data->mode = 0;
        ktf::run_kernel_test(self);

	strcpy(data->expected, "something_else");
	ktf::run_kernel_test(self);

	<normal gtest code>

	...
    }

On the kernel side, a hybrid test is written as a normal kernel test using
the ``TEST()`` macro, and the test must be added using ``ADD_TEST()`` as
usual. Include the user land header file to know the data type of the
out-of-band parameter block. Invoke the macro ``KTF_USERDATA()`` to get a
size validated pointer to the user land provided data. If no data is
available, the test will silently exit. This is by purpose - if the kernel
test is executed from a test program that does not have the associated user
land code, such as for instance ``ktfrun``, it will just appear as a test
with no assertions in it, and not create any errors. If on the other hand the
parameter block does not match in size, an assertion is thrown and the test
exits::

    TEST(foo, hybrid)
    {
	KTF_USERDATA(self, my_params, data);

	...
	if (strcmp(data->expected, "something") == 0)
	   ...
	   EXPECT( ... )

 	...
    }


Running tests and examining results via debugfs
***********************************************

In addition to the netlink interface used by the Googletest integrated frontend code,
we provide debugfs interfaces for examining the results of the
last test run and for running tests which do not require configuration
specification. Individual ktf testsets can be run via::

    cat /sys/kernel/debug/ktf/run/<testset>

Individual tests can be run via::

    cat /sys/kernel/debug/ktf/run/<testset>-tests/<test>

Results can be displayed for the last run via::

    cat /sys/kernel/debug/ktf/results/<testset>

Individual tests can be run via::

    cat /sys/kernel/debug/ktf/results/<testset>-tests/<test>

These interfaces bypasses use of the netlink socket API
and provide a simple way to keep track of test failures.  It can
be useful to log into a machine and examine what tests were run
without having console history available.

In particular::

    cat /sys/kernel/debug/ktf/run/*

...is a useful way of running all KTF tests.
