/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * ktf.h: Defines the KTF user API for kernel clients
 */
#ifndef _KTF_H
#define _KTF_H

#include <linux/completion.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/ptrace.h>
#include "ktf_test.h"
#include "ktf_override.h"
#include "ktf_map.h"
#include "ktf_unlproto.h"

#define	KTF_MAX_LOG			2048

struct ktf_context {
	struct ktf_map_elem elem;  /* Linkage for ktf_map */
	struct ktf_handle *handle; /* Owner of this context */
};

/* type for a test function */
struct ktf_test;

/* state of running test, used to pass to threads spawned by test. */
struct ktf_test_state;

struct ktf_thread {
	int (*func)(void *);
	const char *name;
	struct task_struct *task;
	struct ktf_test_state state;
	struct completion started;
	struct completion completed;
};

typedef void (*ktf_test_adder)(void);

/* Generic setup function for client modules */
void ktf_add_tests(ktf_test_adder f);
int ktf_context_add(struct ktf_handle *handle, struct ktf_context* ctx, const char* name);
const char *ktf_context_name(struct ktf_context *ctx);
struct ktf_context* ktf_find_context(struct ktf_handle *handle, const char* name);
struct ktf_context *ktf_find_first_context(struct ktf_handle *handle);
struct ktf_context *ktf_find_next_context(struct ktf_context* ctx);
void ktf_context_remove(struct ktf_context *ctx);
size_t ktf_has_contexts(struct ktf_handle *handle);
void ktf_context_remove_all(struct ktf_handle *handle);

/* Declare the implicit __test_handle as extern for .c files that use it
 * when adding tests with ADD_TEST but where definition is in another .c file:
 */
extern struct ktf_handle __test_handle;

/* Add/remove/find a context to/from the default handle */
#define KTF_CONTEXT_ADD(__context, name) ktf_context_add(&__test_handle, __context, name)
#define KTF_CONTEXT_REMOVE(__context) ktf_context_remove(__context)
#define KTF_CONTEXT_FIND(name) ktf_find_context(&__test_handle, name)
#define KTF_CONTEXT_GET(name, type) \
	container_of(KTF_CONTEXT_FIND(name), type, k)

/* Part of KTF support for hybrid tests: Safe get the out-of-band user data
 * Silently return if no data (to avoid failing if a generic user program without
 * specific support for the hybrid test is used)
 * Fail if an object of an unecpected size is provided.
 */
#define KTF_USERDATA(__kt_ptr, __priv_datatype, __priv_data)			\
	struct __priv_datatype *__priv_data = (struct __priv_datatype *)__kt_ptr->data; \
	if (!__priv_data) return; \
	ASSERT_LONG_EQ(__kt_ptr->data_sz, sizeof(struct __priv_datatype))


/* KTF support for entry/return probes (via kprobes kretprobes).  We use
 * kretprobes as they support entry/return and do not induce panics when
 * mixed with gkdb usage.
 */

#if (defined(CONFIG_KPROBES) && defined(CONFIG_KRETPROBES) && \
    (defined(CONFIG_X86_64) || defined(CONFIG_ARM) || \
     defined(CONFIG_ARM64) || defined(CONFIG_SPARC)))
#define KTF_PROBE_SUPPORT
#else
static inline int ktf_no_probe_support(void)
{
	twarn("No support for k[ret]probes, or platform not supported.")
	return -ENOTSUPP);
}
#endif

/* Entry/return probe - type is handler type (entry_handler for entry,
 * handler for return), func is function to be probed; probehandler is name
 * of probe handling function we will invoke on entry/return.
 */
#define KTF_PROBE(type, func, probehandler) \
	static int probehandler(struct kretprobe_instance *, struct pt_regs *);\
	static struct kretprobe __ktf_##type##_##probehandler = { \
		.type = probehandler, \
		.data_size = 0, \
		.maxactive = 0, \
		.kp = { \
			.symbol_name    = #func, \
		}, \
	}; \
	static int probehandler(struct kretprobe_instance *ri, \
				struct pt_regs *regs)

#ifdef KTF_PROBE_SUPPORT
#define KTF_REGISTER_PROBE(type, func, probehandler) \
	register_kretprobe(&__ktf_##type##_##probehandler)
#else
#define	KTF_REGISTER_PROBE(type, func, probehandler) \
	ktf_no_probe_support()
#endif

/* Note on the complexity below - to re-use a statically-defined kretprobe for
 * registration, we need to clean up state in the struct kretprobe.  Hence
 * we zero out the kretprobe and re-set the symbol name/handler.  Not doing
 * this means that re-registering fails with -EINVAL.
 */
#define KTF_UNREGISTER_PROBE(type, func, probehandler) \
	do { \
		unregister_kretprobe(&__ktf_##type##_##probehandler); \
		memset(&__ktf_##type##_##probehandler, 0, \
		       sizeof(struct kretprobe)); \
		__ktf_##type##_##probehandler.kp.symbol_name = #func; \
		__ktf_##type##_##probehandler.type = probehandler; \
	} while (0)

#define KTF_ENTRY_PROBE(func, probehandler)     \
	KTF_PROBE(entry_handler, func, probehandler)

#define KTF_REGISTER_ENTRY_PROBE(func, probehandler) \
	KTF_REGISTER_PROBE(entry_handler, func, probehandler)

/* arch-specific calling conventions for kretprobes entry handlers.  Define
 * more args/architectures if needed.
 */
#ifdef KTF_PROBE_SUPPORT
#ifdef CONFIG_X86_64
#define	KTF_ENTRY_PROBE_ARG0		(regs->di)
#define	KTF_ENTRY_PROBE_ARG1		(regs->si)
#endif /* CONFIG_X86_64 */
#ifdef CONFIG_ARM
#define	KTF_ENTRY_PROBE_ARG0		(regs->regs[0])
#define	KTF_ENTRY_PROBE_ARG1		(regs->regs[1])
#endif /* CONFIG_ARM */
#ifdef CONFIG_ARM64
#define	KTF_ENTRY_PROBE_ARG0		(regs->regs[0])
#define	KTF_ENTRY_PROBE_ARG1		(regs->regs[1])
#endif /* CONFIG_ARM64 */
#ifdef CONFIG_SPARC
#define	KTF_ENTRY_PROBE_ARG0		(regs->u_regs[UREG_I0])
#define	KTF_ENTRY_PROBE_ARG1		(regs->u_regs[UREG_I1])
#endif /* CONFIG_SPARC */
#endif /* KTF_PROBE_SUPPORT */

/* For unsupported platforms. */
#ifndef KTF_ENTRY_PROBE_ARG0
#define	KTF_ENTRY_PROBE_ARG0		(0)
#define	KTF_ENTRY_PROBE_ARG1		(1)
#endif

#define KTF_ENTRY_PROBE_RETURN(retval) \
	do { \
		return retval; \
	} while (0)

#define	KTF_UNREGISTER_ENTRY_PROBE(func, probehandler) \
	KTF_UNREGISTER_PROBE(entry_handler, func, probehandler)

/* KTF support for return probes (via kprobes kretprobes) */
#define	KTF_RETURN_PROBE(func, probehandler)	\
	KTF_PROBE(handler, func, probehandler)

#define	KTF_REGISTER_RETURN_PROBE(func, probehandler) \
	KTF_REGISTER_PROBE(handler, func, probehandler)

/* KTF_*RETURN_VALUE() definitions for use within KTF_RETURN_PROBE() {} only. */

#define KTF_RETURN_VALUE()	regs_return_value(regs)

#ifdef KTF_PROBE_SUPPORT
#ifdef CONFIG_X86_64
#define KTF_SET_RETURN_VALUE(value)     regs->ax = (value)
#endif /* CONFIG_X86_64 */
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
#define	KTF_SET_RETURN_VALUE(value)	regs->regs[0] = (value)
#endif /* CONFIG_ARM[64] */
#if defined(CONFIG_SPARC)
#define	KTF_SET_RETURN_VALUE(value)	regs->u_regs[UREG_I0] = (value)
#endif /* CONFIG_SPARC */
#endif /* KTF_PROBE_SUPPORT */

/* For unsupported platforms. */
#ifndef KTF_PROBE_SUPPORT
#define	KTF_SET_RETURN_VALUE(value)	do { } while (0)
#endif /* KTF_PROBE_SUPPORT */

#define KTF_UNREGISTER_RETURN_PROBE(func, probehandler) \
	KTF_UNREGISTER_PROBE(handler, func, probehandler)

#define	KTF_OVERRIDE(func, probehandler) \
	static int probehandler(struct kprobe *, struct pt_regs *);\
	static struct kprobe __ktf_override_##probehandler = { \
		.symbol_name = #func, \
		.pre_handler = probehandler, \
		.post_handler = ktf_post_handler, \
		.fault_handler = NULL, \
		.flags = 0, \
        }; \
        static int probehandler(struct kprobe *kp, struct pt_regs *regs)

#ifdef KTF_PROBE_SUPPORT
#define	KTF_REGISTER_OVERRIDE(func, probehandler) \
	ktf_register_override(&__ktf_override_##probehandler)
#else
#define	KTF_REGISTER_OVERRIDE(func, probehandler) \
	ktf_no_probe_support()
#endif

#define	KTF_UNREGISTER_OVERRIDE(func, probehandler) \
	do { \
		unregister_kprobe(&__ktf_override_##probehandler); \
		memset(&__ktf_override_##probehandler, 0, \
		       sizeof(struct kprobe)); \
		__ktf_override_##probehandler.symbol_name = #func; \
		__ktf_override_##probehandler.pre_handler = probehandler; \
		__ktf_override_##probehandler.post_handler = ktf_post_handler; \
	} while (0)


#define	KTF_OVERRIDE_RETURN \
	do { \
		ktf_override_function_with_return(regs); \
		return 1; \
	} while (0)

#ifdef KTF_PROBE_SUPPORT
#define	KTF_SET_INSTRUCTION_POINTER(regs, value) \
	instruction_pointer_set(regs, (value))
#else
#define	KTF_SET_INSTRUCTION_POINTER(regs, value)	do { } while (0)
#endif

/* Interfaces for creating kthreads in tests. */
#define	KTF_THREAD_INIT(threadname, t) \
	do { \
		(t)->func = threadname; \
		(t)->name = #threadname; \
		(t)->state.self = self; \
		(t)->state.ctx = ctx; \
		(t)->state.iter = _i; \
		(t)->state.value = _value; \
		init_completion(&((t)->started)); \
		init_completion(&((t)->completed)); \
	} while (0)

#define	KTF_THREAD_RUN(t) \
	((t)->task = kthread_run((t)->func, t, (t)->name))

#define KTF_THREAD_STOP(t) \
	do { \
		if ((t)->task) \
			kthread_stop((t)->task); \
	} while (0)

/* Wraps thread execution to supply same variables as test case - this allows
 * us to define assertions etc in thread context.
 */
#define	KTF_THREAD(name) \
	static void __##name(struct ktf_thread *thread, struct ktf_test *self, \
			     struct ktf_context *ctx, int _i, u32 _value); \
	static int name(void *data) \
	{ \
		struct ktf_thread *t = data; \
		complete(&t->started); \
		__##name(t, t->state.self, t->state.ctx, t->state.iter, \
			 t->state.value); \
		complete(&t->completed); \
		return 0; \
	} \
	static void __##name(struct ktf_thread *_thread, struct ktf_test *self,\
			     struct ktf_context *ctx, int _i, u32 _value)

#define	KTF_THREAD_WAIT_STARTED(t)	(wait_for_completion(&((t)->started)))
#define	KTF_THREAD_WAIT_COMPLETED(t)	(wait_for_completion(&((t)->completed)))

u32 ktf_get_assertion_count(void);

/**
 * ASSERT_TRUE() - fail and return if @C evaluates to false
 * @C: Boolean expression to evaluate
 *
 */
#define ASSERT_TRUE(C) do { \
		if (!fail_unless((C))) return;	\
	} while (0)

/**
 * ASSERT_FALSE() - fail and return if @C evaluates to true
 * @C: Boolean expression to evaluate
 */
#define ASSERT_FALSE(C) do { \
		if (!fail_unless(!(C))) return;	\
	} while (0)

/**
 * ASSERT_TRUE_GOTO() - fail and jump to @_lbl if @C evaluates to false
 * @C: Boolean expression to evaluate
 * @_lbl: Label to jump to in case of failure
 */
#define ASSERT_TRUE_GOTO(C,_lbl) {		\
	if (!fail_unless((C))) goto _lbl;\
}

/**
 * ASSERT_FALSE_GOTO() - fail and jump to @_lbl if @C evaluates to true
 * @C: Boolean expression to evaluate
 * @_lbl: Label to jump to in case of failure
 */
#define ASSERT_FALSE_GOTO(C,_lbl) {		\
	if (!fail_unless(!(C))) goto _lbl;\
}

/**
 * ASSERT_TRUE_RETVAL() - fail and return @V if @C evaluates to false
 * @C: Boolean expression to evaluate
 * @V: Value to return on failure
 */
#define ASSERT_TRUE_RETVAL(C, V) do { \
	if (!fail_unless((C))) return V;	\
} while (0)

/**
 * ASSERT_FALSE() - fail and return @V if @C evaluates to true
 * @C: Boolean expression to evaluate
 * @V: Value to return on failure
 */
#define ASSERT_FALSE_RETVAL(C, V) do { \
	if (!fail_unless(!(C))) return V;	\
} while (0)

/**
 * ASSERT_TRUE_CONT() - fail and continue if @C evaluates to false
 * @C: Boolean expression to evaluate
 */
#define ASSERT_TRUE_CONT(C) { \
	if (!fail_unless((C))) continue; \
}

/**
 * ASSERT_FALSE_CONT() - fail and continue if @C evaluates to true
 * @C: Boolean expression to evaluate
 */
#define ASSERT_FALSE_CONT(C) { \
	if (!fail_unless(!(C))) continue; \
}

/**
 * ASSERT_TRUE_BREAK() - fail and break if @C evaluates to false
 * @C: Boolean expression to evaluate
 */
#define ASSERT_TRUE_BREAK(C) { \
	if (!fail_unless((C))) break; \
}

/**
 * ASSERT_FALSE_BREAK() - fail and break if @C evaluates to true
 * @C: Boolean expression to evaluate
 */
#define ASSERT_FALSE_BREAK(C) { \
	if (!fail_unless(!(C))) break; \
}

/**
 * ASSERT_LONG_EQ() - compare two longs, fail and return if @X != @Y
 * @X: Expected value
 * @Y: Actual value
 */
#define ASSERT_LONG_EQ(X, Y) \
	_ck_assert_long_ret(X, ==, Y);

#define ASSERT_LONG_NE(X, Y) \
	_ck_assert_long_ret(X, !=, Y);

#define ASSERT_ADDR_EQ(X, Y) \
	_ck_assert_long_ret((u64)(X), ==, (u64)(Y));

#define ASSERT_ADDR_NE(X, Y) \
	_ck_assert_long_ret((u64)(X), !=, (u64)(Y));

#define ASSERT_INT_EQ(X, Y) \
	_ck_assert_int_ret(X, ==, Y);

#define ASSERT_INT_GT(X, Y) \
	_ck_assert_int_ret(X, >, Y);

/**
 * ASSERT_LONG_EQ() - compare two longs, jump to @_lbl if @X != @Y
 * @X: Expected value
 * @Y: Actual value
 * @_lbl: Label to jump to in case of failure
 */
#define ASSERT_LONG_EQ_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto(X, ==, Y, _lbl)

#define ASSERT_LONG_NE_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto(X, !=, Y, _lbl)

#define ASSERT_ADDR_EQ_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto((u64)(X), ==, (u64)(Y), _lbl)

#define ASSERT_ADDR_NE_GOTO(X, Y, _lbl) \
	_ck_assert_long_goto((u64)(X), !=, (u64)(Y), _lbl)

#define ASSERT_INT_EQ_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, ==, Y, _lbl)

#define ASSERT_INT_GE_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, >=, Y, _lbl)

#define ASSERT_INT_GT_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, >, Y, _lbl)

#define ASSERT_INT_LT_GOTO(X, Y, _lbl) \
	_ck_assert_int_goto(X, <, Y, _lbl)

#define ASSERT_INT_NE(X,Y) \
	_ck_assert_int_ret(X, !=, Y);

#define ASSERT_INT_NE_GOTO(X,Y,_lbl) \
	_ck_assert_int_goto(X, !=, Y, _lbl);

/**
 * EXPECT_TRUE() - fail if @C evaluates to false but allow test to continue
 * @C: Boolean expression to evaluate
 *
 */
#define EXPECT_TRUE(C) fail_unless(C)
#define EXPECT_FALSE(C) fail_unless(!(C))

#define OK_ADDR(X) (X && !IS_ERR(X))

/* Valid kernel address check */
#define EXPECT_OK_ADDR(X) \
	ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - was 0x%Lx", (X))

#define ASSERT_OK_ADDR(X) do { \
		if (!ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - value 0x%Lx", (X))) \
			return;						\
	} while (0)
#define ASSERT_OK_ADDR_GOTO(X,_lbl) do { \
		if (!ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - was 0x%Lx", (X))) \
			goto _lbl;					\
	} while (0)

#define ASSERT_OK_ADDR_BREAK(X) do { \
	if (!ck_assert_msg(OK_ADDR(X), "Invalid pointer '"#X"' - was 0x%Lx", (X))) \
		break; \
	} while (0)

#define EXPECT_INT_EQ(X,Y) _ck_assert_int(X, ==, Y)
#define EXPECT_INT_GT(X,Y) _ck_assert_int(X, >, Y)
#define EXPECT_INT_GE(X,Y) _ck_assert_int(X, >=, Y)
#define EXPECT_INT_LE(X,Y) _ck_assert_int(X, <=, Y)
#define EXPECT_INT_LT(X,Y) _ck_assert_int(X, <, Y)
#define EXPECT_INT_NE(X,Y) _ck_assert_int(X, !=, Y)

#define EXPECT_LONG_EQ(X, Y) _ck_assert_long(X, ==, Y)
#define EXPECT_LONG_NE(X, Y) _ck_assert_long(X, !=, Y)
#define EXPECT_ADDR_EQ(X, Y) _ck_assert_long((u64)(X), ==, (u64)(Y))
#define EXPECT_ADDR_NE(X, Y) _ck_assert_long((u64)(X), !=, (u64)(Y))
#define EXPECT_LONG_GT(X, Y) _ck_assert_long(X, >, Y)
#define EXPECT_LONG_GE(X, Y) _ck_assert_long(X, >=, Y)
#define EXPECT_LONG_LE(X, Y) _ck_assert_long(X, <=, Y)
#define EXPECT_LONG_LT(X, Y) _ck_assert_long(X, <, Y)

#define EXPECT_STREQ(X, Y) _ck_assert_str_eq(X, Y)
#define EXPECT_STRNE(X, Y) _ck_assert_str_ne(X, Y)

extern ulong ktf_debug_mask;

/* Defined debug bits - higher values should represent more
 * verbose categories:
 */
#define T_INFO		0x1
#define T_LIST		0x2
#define T_INTR	      0x200
#define T_INFO_V      0x800
#define T_DEBUG	     0x1000
#define T_MCAST	     0x2000
#define T_TRACE	   0x100000
#define T_DEBUG_V  0x200000

#define tlog(class, format, arg...)	\
	do { \
		if (unlikely((ktf_debug_mask) & (class)))	\
			printk(KERN_INFO \
				   "ktf pid [%d] " "%s: " format "\n", \
				   current->pid, __func__, \
				   ## arg); \
	} while (0)
#define twarn(format, arg...)	\
	do { \
		printk(KERN_WARNING				       \
		       "ktf pid [%d] " "%s: " format "\n",	       \
		       current->pid, __func__,			       \
		       ## arg);				       \
	} while (0)
#define terr(format, arg...)	\
	do { \
		printk(KERN_ERR				       \
		       "ktf pid [%d] " "%s: " format "\n",	       \
		       current->pid, __func__,			       \
		       ## arg);				       \
	} while (0)
#define tlogs(class, stmt_list) \
	do { \
		if (unlikely((ktf_debug_mask) & (class))) { \
			stmt_list;\
		} \
	} while (0)


/* Look up the current address of a potentially local symbol - to allow testing
 * against it. NB! This is a hack for unit testing internal unexposed interfaces and
 * violates the module boundaries and has no fw/bw comp gauarantees, but are
 * still very useful for detailed unit testing complex logic:
 */
void* ktf_find_symbol(const char *mod, const char *sym);

unsigned long ktf_symbol_size(unsigned long addr);

#define ktf_resolve_symbol(mname, sname) \
	do { \
		sname = ktf_find_symbol(#mname, #sname);	\
		if (!sname) \
			return -ENOENT; \
	} while (0)
#endif
