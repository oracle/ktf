7. KTF programming reference
----------------------------

KTF itself contains no tests but provides primitives and data structures to
allow tests to be maintained and written in separate test modules that
depend on the KTF APIs.

KTF API Overview
****************

For reference, the following table lists a few terms and classes of
abstractions provided by KTF. These are kernel side, if not otherwise noted:

+----------------------------+--------------------------------------------------+
| **Item**		     | **description** 				        |
+============================+==================================================+
| Test module		     | A kernel object file (.ko) with ktf tests in it	|
+----------------------------+--------------------------------------------------+
| struct ktf_handle	     | At least 1 per test module.                      |
|		   	     | Use macros KTF_INIT() and KTF_CLEANUP() to set up|
|			     | and tear down handles.				|
+----------------------------+--------------------------------------------------+
| struct ktf_context	     | 0-n per test module - test module specific       |
|		     	     | context for the test, such as eg. a device or    |
|		     	     | another kernel object.		                |
+----------------------------+--------------------------------------------------+
| KTF_INIT()		     | Call this at the global level in the main file   |
|			     | for each test module. Declares an implicit, 	|
|			     | default test handle used by macros which do not  |
|			     | provide a handle argument.			|
+----------------------------+--------------------------------------------------+
| KTF_CTX_INIT()	     | Use this instead of KTF_INIT if the tests require|
|			     | a context to execute. Tests will only show up as |
| 			     | options if a context has been provided.		|
+----------------------------+--------------------------------------------------+
| KTF_HANDLE_INIT(handle)    | Declare a named handle to associate tests and	|
|			     | contexts with. This is an alternative to 	|
|			     | KTF_INIT() to allow the use of separate test 	|
|			     | handles for separate sets of tests.	    	|
+----------------------------+--------------------------------------------------+
| KTF_HANDLE_CTX_INIT(handle)| Equivalent of KTF_CTX_INIT for a named handle	|
+----------------------------+--------------------------------------------------+
| KTF_CLEANUP()		     | Call this in the __exit function to clean up     |
+----------------------------+--------------------------------------------------+
| KTF_CONTEXT_ADD(ctx, name) | Add a new context to the default handle		|
+----------------------------+--------------------------------------------------+
| KTF_CONTEXT_FIND(name)     | Return a struct ktf_context reference to	context	|
| 			     | 'name', if it exists		     		|
+----------------------------+--------------------------------------------------+
| KTF_CONTEXT_GET(name,type) | Return the structure of type 'type' containing   |
| 			     | the ktf_context named 'name', if 'name' exists.  |
+----------------------------+--------------------------------------------------+
| KTF_CONTEXT_REMOVE(ctx)    | Remove a previously added context from KTF	|
+----------------------------+--------------------------------------------------+
| EXPECT_*		     | non-fatal assertions                             |
+----------------------------+--------------------------------------------------+
| ASSERT_*		     | "fatal" assertions that would cause return/goto	|
+----------------------------+--------------------------------------------------+
| TEST(s, n) {...}	     | Define a simple test named 's.n' with implicit 	|
|		     	     | arguments 'ctx' and '_i' for context/iteration.  |
+----------------------------+--------------------------------------------------+
| DECLARE_F(f) {...}	     | Declare a new test fixture named 'f' with        |
|		     	     | additional data structure	                |
+----------------------------+--------------------------------------------------+
| SETUP_F(f, s) {...}	     | Define setup function for the fixture            |
+----------------------------+--------------------------------------------------+
| TEARDOWN_F(f, t) {...}     | Define teardown function for the fixture         |
+----------------------------+--------------------------------------------------+
| INIT_F(f, s, t) {...}      | Declare the setup and tear down functions for the|
|			     | fixture						|
+----------------------------+--------------------------------------------------+
| TEST_F(s, f, n) {...}      | Define a test named 's.n' operating in fixture f	|
+----------------------------+--------------------------------------------------+
| ADD_TEST(n)		     | Add a test previously declared with TEST or	|
| 			     | TEST_F to the default handle.  	   		|
+----------------------------+--------------------------------------------------+
| ADD_LOOP_TEST(n, from, to) | Add a test to be executed repeatedly with a range|
| 		   	     | of values [from,to] to the implicit variable _i	|
+----------------------------+--------------------------------------------------+
| DEL_TEST(n)		     | Remove a test previously added with ADD_TEST	|
+----------------------------+--------------------------------------------------+
| KTF_ENTRY_PROBE(f, h)      | Define function entry probe for function f with  |
| {...}              	     | handler function h. Must be used at global level.|
+----------------------------+--------------------------------------------------+
| KTF_ENTRY_PROBE_RETURN(r)  | Return from probed function with return value r. |
|  			     | Must be called within KTF_ENTRY_PROBE().         |
+----------------------------+--------------------------------------------------+
| KTF_REGISTER_ENTRY_PROBE   | Enable probe on entry to kernel function f	|
| (f, h)                     | with handler h.                                  |
+----------------------------+--------------------------------------------------+
| KTF_UNREGISTER_ENTRY_PROBE | Disable probe on entry to kernel function f      |
| (f, h)		     | which used handler h.                            |
+----------------------------+--------------------------------------------------+
| KTF_RETURN_PROBE(f, h)     | Define function return probe for function f with |
| {..}			     | handler h.  Must be used at a global level.      |
+----------------------------+--------------------------------------------------+
| KTF_RETURN_VALUE()         | Retrieve return value in body of return probe.   |
+----------------------------+--------------------------------------------------+
| KTF_REGISTER_RETURN_PROBE  | Enable probe for return of function f with       |
| (f, h)                     | handler h.                                       |
+----------------------------+--------------------------------------------------+
| KTF_UNREGISTER_RETURN_PROBE| Disable probe for return of function f and       |
| (f, h)                     | handler h.                                       |
+----------------------------+--------------------------------------------------+
| ktf_cov_enable(m, flags)   | Enable coverage analytics for module m.          |
|			     | Flag must be either 0 or KTF_COV_OPT_MEM.        |
+----------------------------+--------------------------------------------------+
| ktf_cov_disable(m)	     | Disable coverage analytics for module m.         |
+----------------------------+--------------------------------------------------+
| KTF_THREAD_INIT(name, t)   | Initialize thread name, struct ktf_thread * t.   |
+----------------------------+--------------------------------------------------+
| KTF_THREAD_RUN(t)          | Run initialized struct ktf_thread * t.           |
+----------------------------+--------------------------------------------------+
| KTF_THREAD_STOP(t)         | Stop thread via kthread_stop()                   |
+----------------------------+--------------------------------------------------+
| KTF_THREAD_WAIT_STARTED(t) | Wait for start of struct ktf_thread * t.         |
+----------------------------+--------------------------------------------------+
| KTF_THREAD_WAIT_COMPLETED  | Wait for completion of struct ktf_thread * t.    |
| (t)                        |                                                  |
+----------------------------+--------------------------------------------------+
| HTEST(s, n) { ... }        | Declares a hybrid test. A correspondingly named  |
| (NB! User mode only!)      | test must be declared using TEST() from kernel   |
|                            | space for the hybrid test to be executed.        |
+----------------------------+--------------------------------------------------+
| KTF_USERDATA(self, type, d)| Declare/get a pointer to user/kernel aux.data    |
| (NB! both kernel and       | for a test that declares such extra data. Used   |
| user space!)               | for hybrid tests.                                |
+----------------------------+--------------------------------------------------+

The ``KTF_INIT()`` macro must be called at a global level as it just
defines a variable ``__test_handle`` which is referred to, and which existence
is assumed to continue until the call to KTF_CLEANUP(), typically done in
the ``__exit`` function of the test module.



Assertions
**********

Below is example documentation for some of the available assertion macros.
For a full overview, see ``kernel/ktf.h``

.. kernel-doc:: kernel/ktf.h
   :internal:
