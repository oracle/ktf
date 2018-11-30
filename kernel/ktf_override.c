/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Alan Maguire <alan.maguire@oracle.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * ktf_override.c: support for overriding function entry.
 */
#include <linux/kprobes.h>
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


/* Prior to Linux 4.19, error exit did not clear active kprobe; as a result,
 * every page fault would fail due to logic in page fault handling activated
 * when a kprobe is active.  We clean up by setting per-cpu variable
 * "current_kprobe" to NULL ourselves.
 */
void ktf_override_function_with_return(struct pt_regs *regs)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	const struct kprobe * __percpu *current_kprobe =
	    raw_cpu_ptr(ktf_find_symbol(NULL, "current_kprobe"));
	if (*current_kprobe)
		*current_kprobe = NULL;
#endif
	regs->ip = (unsigned long)&ktf_just_return_func;
}
NOKPROBE_SYMBOL(ktf_override_function_with_return);
EXPORT_SYMBOL(ktf_override_function_with_return);
