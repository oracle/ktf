// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 Knut Omang <knuto@ifi.uio.no>
 *
 * ktf_kallsyms.h: Access to kernel private symbols
 */

#ifndef _KTF_KALLSYMS_H
#define _KTF_KALLSYMS_H
#include <linux/version.h>

int ktf_kallsyms_init(void);

struct ktf_kernel_internals {
	/* From kallsyms.h: In kernels beyond 5.8 these are not exported to modules */
	unsigned long (*kallsyms_lookup_name)(const char *name);
	int (*kallsyms_on_each_symbol)(int (*fn)(void *, const char *, struct module *,
						 unsigned long),
				       void *data);
	/* From module.h: Look up a module symbol - supports syntax module:name */
	unsigned long (*module_kallsyms_lookup_name)(const char *name);
	/* From kallsyms.h: Look up a symbol w/size and offset */
	unsigned long (*kallsyms_lookup_size_offset)(unsigned long addr,
						     unsigned long *symbolsize,
						     unsigned long *offset);
};

extern struct ktf_kernel_internals ki;
#endif
