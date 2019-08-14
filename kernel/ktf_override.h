// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 *    Author: Alan Maguire <alan.maguire@oracle.com>
 *
 * ktf_override.h: Function override support interface for KTF.
 */
#include <linux/kprobes.h>
#include "ktf.h"

void ktf_post_handler(struct kprobe *kp, struct pt_regs *regs,
		      unsigned long flags);
void ktf_override_function_with_return(struct pt_regs *regs);
int ktf_register_override(struct kprobe *kp);
