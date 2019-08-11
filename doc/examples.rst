6. Example test code
--------------------

Here is a minimal dummy example of a KTF unit test suite that defines
two tests, ``hello_ok`` and ``hello_fail``. The test is in the examples
directory and is built with KTF:

.. literalinclude:: ../examples/hello.c
   :language: c

To run the test, cd to your KTF build tree and insmod the ktf module and
the module that provides the test::

    insmod kernel/ktf.ko
    insmod examples/hello.ko

Now you should be able to run one or more of the tests by running the
application ``ktfrun`` built in ``user/ktfrun``. You should be able to run
that application as an ordinary user::

    ktfrun --gtest_list_tests
    ktfrun --gtest_filter='*fail'
    ktfrun --gtest_filter='*ok'

There are more examples in the examples directory. KTF also includes a
``selftest`` directory used to test/check the KTF implementation itself.
