// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Alan Maguire <alan.maguire@oracle.com>
 *
 * ktf_override.c: support for overriding function entry.
 */
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include "ktf.h"
#include "ktf_override.h"

asmlinkage void ktf_just_return_func(void);

asm(
	".type ktf_just_return_func, @function\n"
	".globl ktf_just_return_func\n"
	"ktf_just_return_func:\n"
	"	ret\n"
	".size ktf_just_return_func, .-ktf_just_return_func\n"
);

void ktf_post_handler(struct kprobe *kp, struct pt_regs *regs,
		      unsigned long flags)
{
	/*
	 * A dummy post handler is required to prohibit optimizing, because
	 * jump optimization does not support execution path overriding.
	 */
}
EXPORT_SYMBOL(ktf_post_handler);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
static void *ktf_find_current_kprobe_sym(void)
{
	return ktf_find_symbol(NULL, "current_kprobe");
}

/* Prior to Linux 4.19, error exit does not clear active kprobe; as a result,
 * every page fault would fail due to logic in page fault handling activated
 * when a kprobe is active.  We clean up by setting per-cpu variable
 * "current_kprobe" to NULL ourselves.
 */
#endif
void ktf_override_function_with_return(struct pt_regs *regs)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	const struct kprobe __percpu **current_kprobe_ref;
	void *current_kprobe_sym = ktf_find_current_kprobe_sym();

	if (!current_kprobe_sym)
		return;

	preempt_disable();
	current_kprobe_ref = raw_cpu_ptr(current_kprobe_sym);
	if (*current_kprobe_ref)
		*current_kprobe_ref = NULL;
	preempt_enable();
#endif
	KTF_SET_INSTRUCTION_POINTER(regs, (unsigned long)&ktf_just_return_func);
}
EXPORT_SYMBOL(ktf_override_function_with_return);
NOKPROBE_SYMBOL(ktf_override_function_with_return);

int ktf_register_override(struct kprobe *kp)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	/* We can only support override if we can fix current_kprobe setting in
	 * ktf_override_function_with_return().  To do that we need access to
	 * the "current_kprobe" symbol address.  If a kernel has been compiled
	 * with CONFIG_KALLSYMS_ALL=n this will not be accessible, so fail at
	 * registration time.
	 */
	if (!ktf_find_current_kprobe_sym())
		return -ENOTSUPP;
#endif
	return register_kprobe(kp);
}
EXPORT_SYMBOL(ktf_register_override);
