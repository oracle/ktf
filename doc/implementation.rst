
2. Implementation
-----------------

KTF consists of a kernel part and a user part. The role of the user part is to query the kernel
for available tests, and provide mechanisms for executing a selected set or all the available
tests, and report the results. The core ktf kernel module simply provides some APIs to write
assertions and run tests and to communicate about tests and results with user mode.
A simple generic Netlink protocol is used for the communication.

User mode implementation
************************

Since test filtering and reporting is something existing unit test frameworks for
user space code already does well, the implementation of KTF simply leverages that.
The current version supports an integration with gtest (Googletest), which provides a lot of
these features in a flexible way, but in principle alternative implementations could
use the reporting of any other user level unit test framework. The use of gtest also allows this
documentation to be shorter, as many of the features in gtest are automatically available for KTF as well.
More information about Googletest features can be found here: https://github.com/google/googletest

Kernel mode implementation
**************************

The kernel side of KTF implements a simple API for tracking test modules,
writing tests, support functions and and a set of assertion macros, some
tailored for typical kernel usage, such as ``ASSERT_OK_ADDR_GOTO()``
as a kernel specific macro to check for a valid address with a label to jump to if the
assertion fails. After all as we are still in the kernel, tests would always need to clean up for
themselves even though in the context of ktf.

KTF supports two distinct classes of tests:

* Pure kernel mode tests
* Hybrid tests

Pure kernel mode tests are tests that are fully implemented in kernel space.
This is the most straightforward mode and resembles ordinary user land unit testing
in kernel mode. If you only have kernel mode tests, you will only ever need one user level program
similar to user/ktfrun.cpp, since all test development takes place on the kernel side.

Hybrid tests are for testing and making assumptions about the user/kernel communication, for instance
if a parameter supplied from user mode is interpreted the intended way when it arrives at it's kernel
destination. For such tests you need to tell ktf (from user space) when the kernel part of the test
is going to be executed - this can happen multiple times depending on your test needs.
Apart from that it works mostly like a normal gtest user level test.

Kernel integration of KTF or KTF as a separate git project?
***********************************************************

Yes. A lot of test infrastructure and utilities for the Linux kernel
is implemented as part of the linux kernel git project.
This has some obvious benefits, such as

* Always being included
* When APIs are changed, test code can be updated atomically with the rest of the kernel
* Higher visibility and easier access
* Easier integration with internal kernel interfaces useful for testing.

On the other hand providing KTF as a separate project allows

* With some use of ``KERNEL_VERSION`` and ``LINUX_VERSION_CODE``, up-to-date KTF code and tests
  can be allowed to work across kernel versions.
* This in turn allows a single set of newly developed tests to be
  simultaneously tested against multiple older kernels, possibly
  detecting more bugs, or instances of bugs not backported.

So we will continue to support both, and have work in progress to simplify
the maintenance and synchronization of the two versions, and allow the
additional tooling to extend to KTF client test suites as well.
