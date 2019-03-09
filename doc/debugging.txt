8. Debugging KTF
--------------------

Structured debugging/tracing by printf
======================================

The kernel side of KTF implements it's own simple printk based logging
abstraction. You can create log entries by means of calls to the macro
tlog(level, format,...) - level is a bitmask where individual bits
represents log classes. You can read and set this bitmask via a
writable ``/sys`` file::

  # cat /sys/module/ktf/parameters/debug_mask
  0x1
  echo 0x3 > /sys/module/ktf/parameters/debug_mask
  # cat /sys/module/ktf/parameters/debug_mask
  0x3

The default value of ``debug_mask`` is 0x1 = INFO level.
Bits are by convention supposed to be
ordered by verbosity, with the lowest bits reserved for low volume,
important messages, while the higher bist are left for more verbose
debugging. You can also use this mechanism for simple debugging of KTF's
interaction with your tests, as the core KTF code contains some log
statements to make it easier to follow and debug involved
instances of KTF objects.

Similarly, the user library implementing the interaction with the
user land test runner can log details about this. You can enable such
logging by providing a similar bitmask via the environment variable
KTF_DEBUG_MASK.

Debugging fatal errors in tests
===============================

So your KTF test crashed the kernel? Let's see how you can use crash to
examine KTF test cases, individual test logs and see which test is running.

First step is we need to load symbols for KTF.  To get text, data and
bss section locations for the ktf module (assuming it's currently
loaded)::

	# cd /sys/module/ktf/sections
	# cat .text .data .bss
	0xffffffffa0bdb000
	0xffffffffa0bdf000
	0xffffffffa0bdf7a0

Now run crash on your corefile (or /proc/kcore for a live kernel)::

	# crash <path-to-Symbol.map> <path-to-vmlinux> <path-to-core>
	crash> add-symbol-file /path/to/kernel/ktf.ko 0xffffffffa0bdb000 -s .data 0xffffffffa0bdf000 -s .bss 0xffffffffa0bdf7a0

Now we can see the global test_cases rbtree via the handy
"tree" command. It displays an rbtree, and because test_cases
is an rbtree under the hood we can display the set of test
cases as follows::

	crash>  tree -t rbtree -s ktf_case test_cases
	ffff88036f710c00
	struct ktf_case {
	  kmap = {
	    node = {
	      __rb_parent_color = 1,
	      rb_right = 0x0,
	      rb_left = 0x0
	    },
	    key = "selftest\000cov\000probereturn\000probeentry\000wrongversion\000dummy\000simplemap",
	    map = 0xffffffffa0bdd1a0 <test_cases>,
	    refcount = {
	      refcount = {
	        counter = 2
	      }
	    }
	  },
	  tests = {
	    root = {
	      rb_node = 0xffff880250ac4a00
	    },
	    size = 5,
	    lock = {
	      {
	        rlock = {
	          raw_lock = {
	            {
	              head_tail = 655370,
	              tickets = {
	                head = 10,
	                tail = 10
	              }
	            }
	          }
	        }
	      }
	    },
	    elem_comparefn = 0x0,
	    elem_freefn = 0xffffffffa0bd8760 <ktf_test_free>
	  },
	  debugfs = {
	    debugfs_results_testset = 0xffff88021a18a3c0,
	    debugfs_results_test = 0xffff88021a18aa80,
	    debugfs_run_testset = 0xffff88021a18a300,
	    debugfs_run_test = 0xffff88021a18a840
	  }
	}

Here we had 1 test case - from the "key" field
we can see it is called "selftest" - in fact it is
KTF's self tests. Within that one test cases we see
the rbtree for the indivdual selftest tests has a root
rb_node::

	  tests = {
	    root = {
	      rb_node = 0xffff880250ac4a00
	    },

By printing _that_ tree of ktf_test structures from
root node (-N) 0xffff880250ac4a00 we can see our
individual tests::

	crash> tree -t rbtree -s ktf_test -N 0xffff880250ac4a00
	ffff880250ac4a00
	struct ktf_test {
	  kmap = {
	    node = {
	      __rb_parent_color = 1,
	      rb_right = 0xffff880250ac5b00,
	      rb_left = 0xffff880250ac5d00
	    },
	    key = "probeentry\000wrongversion\000dummy\000simplemap\000\000\000\000\000\020\276\240\377\377\377\377 \020\276\240\377\377\377\377@\020\276\240\377",
	    map = 0xffff88036f710c68,
	    refcount = {
	      refcount = {
	        counter = 2
	      }
	    }
	  },
	  tclass = 0xffffffffa0be41a4 "selftest",
	  name = 0xffffffffa0be41bd "probeentry",
	  fun = 0xffffffffa0be1920,
	  start = 0,
	  end = 1,
	  skb = 0xffff88003fc03800,
	  log = 0xffff88003fa58000 "",
	  lastrun = {
	    tv_sec = 1506072537,
	    tv_nsec = 289494591
	  },
	  debugfs = {
	    debugfs_results_testset = 0x0,
	    debugfs_results_test = 0xffff88021a18ac00,
	    debugfs_run_testset = 0x0,
	    debugfs_run_test = 0xffff88021a18af00
	  },
	  handle = 0xffffffffa0be5480
	}
	ffff880250ac5d00
	struct ktf_test {
	  kmap = {
	    node = {
	      __rb_parent_color = 18446612142257621505,
	      rb_right = 0x0,
	      rb_left = 0xffff880250ac4b00
	    },
	    key = "dummy\000simplemap\000\000\000\000\000\020\276\240\377\377\377\377 \020\276\240\377\377\377\377@\020\276\240\377\377\377\377`\020\276\240\377\377\377\377\200\020\276\240\377\377\377\377\320\020\276\240\377",
	    map = 0xffff88036f710c68,
	    refcount = {
	      refcount = {
	        counter = 2
	      }
	    }
	  },
	  tclass = 0xffffffffa0be41a4 "selftest",
	  name = 0xffffffffa0be41d5 "dummy",
	  fun = 0xffffffffa0be10f0,
	  start = 0,
	  end = 1,
	  skb = 0xffff88003fc03800,
	  log = 0xffff88003fa59800 "",
	  lastrun = {
	    tv_sec = 1506072537,
	    tv_nsec = 289477354
	  },
	  debugfs = {
	    debugfs_results_testset = 0x0,
	    debugfs_results_test = 0xffff88021a18a900,
	    debugfs_run_testset = 0x0,
	    debugfs_run_test = 0xffff88021a18a9c0
	  },
	  handle = 0xffffffffa0be5480
	}
	...
	crash>


The "log" fields are empty as each test passed, but we can
see from the "lastrun" times when the tests were run.
Logs will contain assertion failures etc in case of failure.

Note that each test has a "handle" field also - this is
the KTF handle which was used to register the test. Each
handle also shows the currently-executing (if in the middle
of a test run) test associated with it, so if we want to
see where test execution was we can simply print the handle::

	crash> print *(struct ktf_handle *)0xffffffffa0be5480
	$13 = {
	  test_list = {
	    next = 0xffffffffa0be5480,
	    prev = 0xffffffffa0be5480
	  },
	  handle_list = {
	    next = 0xffffffffa0be5490,
	    prev = 0xffffffffa0be5490
	  },
	  ctx_map = {
	    root = {
	      rb_node = 0x0
	    },
	    size = 0,
	    lock = {
	      {
	        rlock = {
	          raw_lock = {
	            {
	              head_tail = 0,
	              tickets = {
	                head = 0,
	                tail = 0
	              }
	            }
	          }
	        }
	      }
	    },
	    elem_comparefn = 0x0,
	    elem_freefn = 0x0
	  },
	  id = 0,
	  version = 4294967296,
	  current_test = 0x0
	}
	crash>

In this case current_test is NULL, but if we crashed in the
middle of executing a test it would show us which struct ktf_test *
it was.
