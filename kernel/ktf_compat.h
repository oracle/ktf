// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_compat.h: Backward compatibility support
 */

/*
 * At any time we want to keep KTF code and users
 * as closely as possible compatible with the latest kernel APIs.
 * This file allows the main code paths to be clean of ifdefs
 * while still allowing KTF and new and old tests to be compatible
 * with older kernel versions.

 * Please add wrapper macros and functions
 * here as needed to keep old versions compiling while
 * making the code compile with newer kernels:
 */

#ifndef _KTF_COMPAT_H
#define _KTF_COMPAT_H

#if (KERNEL_VERSION(4, 11, 0) > LINUX_VERSION_CODE)
#define refcount_read atomic_read
#endif

#if (KERNEL_VERSION(4, 6, 0) > LINUX_VERSION_CODE)
#define nla_put_u64_64bit(m, c, v, x) nla_put_u64(m, c, v)
#endif

#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
static inline void *nla_memdup(const struct nlattr *src, gfp_t gfp)
{
	return kmemdup(nla_data(src), nla_len(src), gfp);
}
#endif

#if (KERNEL_VERSION(5, 2, 0) > LINUX_VERSION_CODE)
static inline unsigned int stack_trace_save(unsigned long *store, unsigned int size,
					    unsigned int skipnr)
{
	struct stack_trace trace;

	trace.nr_entries = 0;
	trace.entries = store;
	trace.max_entries = size;
	trace.skip = skipnr;
	save_stack_trace(&trace);
	return trace.nr_entries;
}
#endif

#if (KERNEL_VERSION(5, 11, 0) > LINUX_VERSION_CODE)
#define nla_strscpy nla_strlcpy
#endif

#endif
