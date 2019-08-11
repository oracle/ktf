:Author: Knut Omang <knut.omang@oracle.com>
:Last Updated: Alan Maguire <alan.maguire@oracle.com>

1. Background and motivation
----------------------------

Kernel Test Framework (KTF) implements a unit test framework for the Linux kernel.
There's a wide selection of unit test frameworks available for normal user land
code testing, but we have so far not seen any similar frameworks that can be used
with kernel code, to test details of both exported and non-exported kernel APIs.
The hope is that providing an easy to use and convenient way to write simple unit
tests for kernel internals, that this can promote a more test driven approach to
kernel development, where appropriate.

An important design goal is to make KTF in a way that it lend itself well to a normal kernel
developer cycle, and that it integrates well with user land unit testing, to allow kernel and
user land tests to behave, look and feel as similar as possible. This should hopefully make it
more intuitive to use as well as more rewarding. We also believe that even a kernel test that
passes should have a nice, easy to read and pleasant output, and that a test framework must have
good observability, that is good mechanisms for debugging what went wrong, both in case of bugs
in the tests and and the test framework itself.

KTF is designed to test the kernel in the same ways it runs. This means we want to stay away from
changing configuration options, or otherwise make changes that makes it hard to logically tell
from a high level perspective whether the kernel with KTF is really logically "the same" as the
kernel our users are exposed to. Of course we all know that it is very hard to test anything
without affecting it, with quantum mechanics as the extreme, but at least we want to make an
effort to make the footprint as small as possible.

KTF tests kernel code by running tests in kernel context - or in the case of hybrid tests - in
both user- and kernel contexts. Doing this ensures that we test kernel codepaths in a real way,
without emulating a kernel execution environment. This gives us vastly more control over what
tests can do versus user-space driven testing, and increases confidence that what the tests test
matches what the kernel does since the test execution environment is identical.

KTF is a product of a refactoring of code used as part of test driven development of a Linux
driver for an Infiniband HCA. It is in active use for kernel component testing within Oracle.

Test driven development
***********************

Unit testing is an important component of Test driven development (TDD).
The idea of test driven development is that when you have some code to write,
whether it is a bug to fix, or a new feature or enhancement, that you start by writing
one or more tests for it, have those tests fail, and then do the actual development.

Typically a test driven development cycle would have several rounds of development and
test using the new unit tests, and once the (new) tests pass, you would
also run all or a suitable subset (limited by execution time) of the old tests to verify
that you have not broken any old functionality by the new code.

At this stage it is important that the tests that are run can be run quickly to allow
them to be actively used in the development cycle. When time comes for
submission of the code, a full, extensive set of both the tests the developer thinks
can touch the affected code *and* all the other tests should be run, and a longer time
to run tests can be afforded.

KTF tries to support this by using the module system of the kernel to support
modularized test suites, where a user only need to insmod the test subsets that he/she wants
to use right then. Different test suites may touch and require different kernel APIs and have
lots of different module and device requirements. To enable as much reuse of the functionality
of code developed within KTF it is important that any piece of test code has as few dependencies
as possible.

Good use cases for KTF
**********************

Unit testing is at it's most valuable when the code to test is relatively error prone, but still
might be difficult to test in a systematic and reproducable way from a normal application level.
It can be difficult to trigger corner cases from a high abstraction layer,
the code paths we want to exercise might only be used occasionally, or we want to exercise
that error/exception scenarios are handled gracefully.

KTF comes to it's strength in testing kernel APIs that are fairly integrated into the kernel,
and depend upon lots of components, making them difficult or error prone to mock. Good examples
are module APIs not easily testable from user land. Exported module APIs are usually only used
by one or a few other kernel clients, and hitting buggy corner cases with these might be hard or
impossible. This typically leads to bugs detected "down the road", when some new client appears
and starts using the API in a new way, or instabilities that go undetected because underlying
semantics that the implementation implicitly depend upon changes in subtle ways.

KTF can use mechanisms such as KTF probes in cases where calls to other functions needs to be
intercepted and/or modified to create the right test condition, whether it means waiting for a
potential race condition to occur, or return an error value, or just collect state to make assertions.

Typical classical use cases that lend itself well to unit testing are simple APIs with a relativ
complex implementation - such as container implementations. Typical kernel examples of these
in the kernel are scatterlist, rbtree, list, XArray etc. When testing the base implementations of such
containers, bringing them entirely out into user space and compiling them standalone require some
additional work up-front to implement mock interfaces to the services provided by the kernel,
but may nonetheless be rewarding in the longer run, as such tests have at it's disposal the whole
arsenal of user land tools, such as gdb, valgrind etc. This, however does not guarantee against
wrong use of a container, such as with interactions between a container and a driver
datastructure.

Testing the *instantiations* of these container implementations inside drivers or
the kernels's own internals might not be that easy with a user land approach, as it very quickly
requires a prohibitive amount of mock interfaces to be written. And even when such mock
interfaces can be written, one cannot be sure that they implement exactly the same as the
environment that the code executes in within the kernel. Having the ability to make tests within
a release kernel, even run the same tests against multiple such kernels is something KTF
supports well. Our experience is that even error scenarios that are hard to reproduce by
running applications on the kernel can often be reproduced with a surprisingly small
number of lines of code in a KTF test, once the problem is understood. And writing that code can
be a very rewarding way of narrowing down a hard bug.

When *not* to use KTF
*********************

Writing kernel code has some challenges compared to user land code.
KTF is intended for the cases where it is not easy to get coverage by writing
simple tests from user land, using an existing rich and well proven user land unit test
framework.

Why *you* would want to write and run KTF tests
***********************************************

Besides the normal write test, write code, run test cycle of development and the obvious benefits of
delivering better quality code with fewer embarrassments, there's a few other upsides from
developing unit test for a particular area of the kernel:

* A test becomes an invariant for how the code is supposed to work.
  If someone breaks it, they should detect it and either document the changes that caused the breakage
  by fixing the test or realize that their fix is broken before you even get to spend time on it.

* Kernel documentation while quite good in some places, does not always
  cover the full picture, or you might not find that sentence you needed while looking for it.
  If you want to better understand how a particular kernel module actually works, a good way is to
  write a test that codes your assumptions. If it passes, all is well, if not, then you have gained some
  understanding of the kernel.

* Sometimes you may find yourself relying on some specific feature or property of the kernel.
  If you encode a test that guards the assumptions you have made, you will capture if someone
  changes it, or if your code is ported to an older kernel which does not support it.
