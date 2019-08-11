
5. KTF Basic Concepts
---------------------

Tests and test suites
*********************

The simplest form of test is to just specify::

	TEST(suite_name, test_name)
	{
		<test code and assertions go here>
	}

A KTF test is declared with TEST() or TEST_F(), which both
takes both a test suite name and a test name, which are two different
name spaces. Consequently, each test belongs to one test suite, and
the test suites are created based on what tests that exists.
A test suite is just a container of tests which in user space
contributes to the extended name of a test. Test names must be
unique within a suite, and test names must also be unique within a
source file, since the test name is the only parameter needed
when adding a test.

All tests must be added using ADD_TEST or ADD_LOOP_TEST to be visible
to KTF's runtime framework. This allows tests to be declared while
under development, but not added (or the ADD_TEST could be commented
out) if the test or the kernel module under test is not ready
yet for some reason.

Test fixtures
*************

As in other unit test frameworks, a test fixture is a mechanism to
allow several tests to run under the same conditions, in that setup
and teardown is done before and after each test. In KTF a test fixture
must first be declared with DECLARE_F() which takes a fixture name
followed by a list of attributes and an end brace, and initialized
with INIT_F() which takes the fixture name and a setup and teardown
function to be defined subsequently. Note that there are
no start brace, which is intentional::

	DECLARE_F(a_fixture)
		int value;
		struct my_details;
		...
	};
        INIT_F(a_fixture, a_setup, a_teardown);

Then to the implementation of the fixture, in the form of actual setup and
a teardown functions that may operate on the attributes of the fixture::

	SETUP_F(a_fixture, a_setup)
	{
		a_fixture->value = 42; /* or whatever.. */
		<other actions needed to set up>
		/* If everything went well during setup: */
		a_fixture->ok = true;
	}

	TEARDOWN_F(a_fixture, a_teardown)
	{
		<necessary cleanup code>
	}

Now individual tests that uses this fixture can be declared with::

	TEST_F(a_fixture, suite_name, test_name)
	{
		<test code>
	}

Contexts
********

A context provides a way to instantiate a test in multiple ways.
A typical use case is if you have multiple similar devices
you want to run a set of tests on. Another use case could be that
you want to run a set of tests under different configurations.

You are free to let the number and names of these contexts
vary as to where you run your test. For the devices use case, you can
have the init function loop through all available devices, to identify
the ones the tests applies to, then instantiate a context for each
device, possibly using the device name for trackability. The context
names will be prepended to the test name and the number of available
tests will be multipled by the number of contexts.

Note that the state of a context persists through the whole "life" of
the module (until it gets unloaded) so it can be used to store more
long term bookeeping in addition to any configuration information.
The test writer must make sure that subsequent runs of the test suite
(or parallel runs!) does not interfere with
each other. Similar to fixtures, there's a generic part that KTF uses,
and it can be extended the normal way. Make sure to declare the
test specific context struct type with an element named::

	struct ktf_context k;

typically as the first element of the struct, then you can continue
with whatever other datastructure desired. A test module can declare
and use as many contexts as desired. Note that contexts are associated
with and unique within a ``handle`` (see below). So if you need
to use a different set of contexts for different tests, you need to
put these contexts and tests into different handles.

A context can be added using something like::

	KTF_CONTEXT_ADD(&my_struct.k, "mycfg")

where the first argument is a reference to the ktf_context structure
within the test specific structure, and the second argument is a text
name to use to refer to the context. Once one or more contexts exists
for a handle, tests for that handle will show up with names postfixed
by the context name, and there will be a distinct version of the test
for each context, e.g if a handle has contexts named ``c1`` and
``c2``, and tests declared with ``TEST(x, t1)`` and ``TEST(x, t2)``,
then this will manifest as 4 tests::

	x.t1_c1
	x.t1_c2
	x.t2_c1
	x.t2_c2

Tests that depends on having a context
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
It is likely that once you have a set of tests that uses contexts,
that they also depends on a context being available, e.g. that the
``ctx`` variable inside a test points to a non-nil context. One example
use case for contexts would be a certain class of device. Such a
device might or might not be available in the system. If one or more
devices are available, you might want to have tests named
``c.t1_dev1`` and ``c.t1_dev2`` where ``dev1`` and ``dev2`` are the
device names for these devices in the system, but never have any
``c.t1`` test without a device. You can
enforce this by letting tests associated with a handle requiring a
context to even show up as a test in the list or be available for
execution. Instead of using ``KTF_INIT()`` or ``KTF_HANDLE_INIT()``,
use ``KTF_CTX_INIT()`` or ``KTF_HANDLE_CTX_INIT()``.

Configurable contexts
~~~~~~~~~~~~~~~~~~~~~
Sometimes it might be useful to be able to configure a context for the
execution of some (or all) of the tests using the context.
This can be because the system the tests are running on might have
different hardware or software capabilities, or might rely on
differing device or network setup or naming. Typically we want a unit
test suite to have as little configuration and parameterization as
possible, so recommended use is for parameters that is not directly
related to the operation of the test, but more for situations where
parameters outside the test itself needs to be set up, such as connect
details for a network service to test against, or a peer unit test
process for network related tests that require more than one
system to run. To specify a configurable context, use::

	int my_cfg_callback(struct ktf_context *ctx, const void* data, size_t data_sz);

	KTF_CONTEXT_ADD_CFG(&my_struct.k, "mycfg", my_cfg_callback, type_id)

The ``data`` pointer (and it's length) should be provided from user
space, and it is up to the test specific user space and kernel space
code to decide with the configuration is all about. If 0 is returned,
KTF considers the context to be configured, otherwise it will retain
it's current state, which will initially be unconfigured.
The callback return value is stored as an errno value in ``ktf_context`` in the
variable ``config_errno``, which will initially be set to ``ENOENT``,
to indicate unconfigured. The test can use this value
to decide what to do, such as failing with a message about missing
configuration or just silently pass and ignore the case if not
configured. The ``type_id`` parameter is used as a unique
identifier for the kernel side to decide how to interpret the
parameter, which is useful if different contexts wants to implement
very different configuration options. It also allows two different
test modules to use the same context names but with different
parameters by using different context types.

In the user space part of the test, configuration information
can be set for a context using::

	KTF_CONTEXT_CFG(name, type_id, parameter_type, parameter_ref)

A simple example of a configurable test can be seen in
the selftests test in ``selftest/context.c`` (kernel part) and
``user/context.cpp`` (user part) and the header file
``selftest/context_self.h`` shared between user space and kernel space.

Context types and user space created contexts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Contexts belong to a ``context type``, which is a mechanism to group
contexts into types that have similar properties. It is up to the
kernel test module using these contexts what the meaning of this is,
but a simple semantics can be that all contexts of a certain type has
the same parameter block and the type ID can be used to check what
type of context it is before trying to resolve or verify the
parameters. For contexts pre-created by the kernel module, type IDs
can be freely selected and does not have any further meaning beyond
this.

Context types can however also be used to selectively allow user
space to dynamically create new contexts of a specific type. To enable
such functionality, the kernel test module will need to enable it for
one or more context types. This is done by means of the following call::

	ktf_handle_add_ctx_type(struct ktf_handle *handle,
				struct ktf_context_type *type)

kernel side call, which tells KTF that a new context type with a given type ID
permits user applications to create new contexts. This is useful for
instance if user parameters or other information most easily
obtainable from user land at test runtime is most easily available
from user space.

Handles
*******

Unlike user land unit test frameworks, which can rely on everything
being cleaned up when the test program finished, KTF and test writers
must pay the normal kernel level attention to allocations, references and
deallocations.

KTF itself uses the concept of a *handle* to track tests,
test suites and contexts associated with a kernel module.
Contexts are also associated with a handle. Since the availability of
contexts for a handle determines the availability of tests and the
naming of them, it can be useful to have separate spaces for tests
that relies on some context and tests that do not, to avoid
aggregating up multiple test cases that are identical.
Handles thus also have a namespace effect in that it is possible to
have two contexts with the same name, and possibly a different type,
by putting them in different handles.

The simplest mode of usage is for each module to use KTF_INIT() and
KTF_CLEANUP() in it's __init and __exit functions. KTF_INIT implicitly declares and
initializes a global handle __test_handle that gets cleaned up again
in KTF_CLEANUP, making the handle something a test developer does not
need to think too much about. However, sometimes a KTF kernel module
may be such organized that it makes sense to use more than one handle.
KTF allows the creation/cleanup of explicitly named handles by means of
KTF_HANDLE_INIT(name) and KTF_HANDLE_CLEANUP(name). This can be used
as an alternative to KTF_INIT()/KTF_CLEANUP() but requires the use of
ADD_TEST_TO(handle, testname) instead of the normal ADD_TEST(testname)
for adding tests to be executed.
