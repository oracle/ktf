// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 Knut Omang <knuto@ifi.uio.no>
 *
 * ktf_kallsyms.c: Access to kernel private symbols for bootstrapping
 *
 * With workaround to bootstrap access to kallsyms_lookup_name itself
 * after it was made inaccessible to modules in
 *   commit 'kallsyms: unexport kallsyms_lookup_name() and kallsyms_on_each_symbol()'
 * Inspired by https://github.com/zizzu0/LinuxKernelModules
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include "ktf.h"
#include "ktf_kallsyms.h"

struct ktf_kernel_internals ki;

#if (KERNEL_VERSION(5, 8, 0) <= LINUX_VERSION_CODE)

static off_t ktf_kallsyms_offset = 0;
static unsigned long kallsyms_lookup_name_probe_entry = 0;

KTF_ENTRY_PROBE(kallsyms_lookup_name, aln_init)
{
	kallsyms_lookup_name_probe_entry = instruction_pointer(regs);
	KTF_ENTRY_PROBE_RETURN(0);
}

KTF_ENTRY_PROBE(kfree, verif)
{
	ktf_kallsyms_offset = (off_t)instruction_pointer(regs) - (off_t)kfree;
	KTF_ENTRY_PROBE_RETURN(0);
}

int ktf_kallsyms_bootstrap(void)
{
	/* Register a probe on kallsyms_lookup_name to get the address where the probe entered: */
	int stat = KTF_REGISTER_ENTRY_PROBE(kallsyms_lookup_name, aln_init);
	if (stat)
		goto init_failed;

	/* Determine the offset from the function the probe is on to the
	 * instruction pointer when we enter the probe by using a probe on a
	 * known symbol: kfree() can be safely called with 0 without side effects
	 * and is unlikely to change definition in a future kernel.
	 *
	 * By registering a second probe we also implicitly trigger a call to the first
	 * probe since kallsym_lookup_name is used internally to find the address of
	 * the function to probe, here kfree:
	 */
	stat = KTF_REGISTER_ENTRY_PROBE(kfree, verif);
	if (stat) {
		KTF_UNREGISTER_ENTRY_PROBE(kallsyms_lookup_name, aln_init);
		goto init_failed;
	}

	/* Now make sure kfree() has been called at least once with the probe:
	 * (it likely has been called several times already but that's no big deal)
	 */
	kfree(0);
	KTF_UNREGISTER_ENTRY_PROBE(kfree, verif);
	KTF_UNREGISTER_ENTRY_PROBE(kallsyms_lookup_name, aln_init);

	ki.kallsyms_lookup_name = (void*)(kallsyms_lookup_name_probe_entry - ktf_kallsyms_offset);
	if (ki.kallsyms_lookup_name) {
		tlog(T_DEBUG, "Got kallsyms_lookup_name at %p", ki.kallsyms_lookup_name);
	} else {
		terr("Unable to access the address of 'kallsyms_lookup_name'");
		return -ENODEV;
	}
	return 0;
init_failed:
	terr("Failed to set up kallsyms access");
	return stat;
}
#endif

/* Support for looking up kernel/module internal symbols to enable testing.
 * A NULL mod means either we want the kernel-internal symbol or don't care
 * which module the symbol is in.
 */
void *ktf_find_symbol(const char *mod, const char *sym)
{
	char sm[200];
	const char *symref;
	unsigned long addr = 0;

	if (mod) {
		sprintf(sm, "%s:%s", mod, sym);
		symref = sm;
	} else {
		/* Try for kernel-internal symbol first; fall back to modules
		 * if that fails.
		 */
		symref = sym;
		addr = ki.kallsyms_lookup_name(symref);
	}
	if (!addr)
		addr = ki.module_kallsyms_lookup_name(symref);
	if (addr) {
		tlog(T_DEBUG, "Found %s at %0lx", sym, addr);
	} else {
#ifndef CONFIG_KALLSYMS_ALL
		twarn("CONFIG_KALLSYMS_ALL is not set, so non-exported symbols are not available");
#endif
		tlog(T_INFO, "Fatal error: %s not found", sym);
		return NULL;
	}
	return (void *)addr;
}
EXPORT_SYMBOL(ktf_find_symbol);

unsigned long ktf_symbol_size(unsigned long addr)
{
	unsigned long size = 0;

	(void)ki.kallsyms_lookup_size_offset(addr, &size, NULL);

	return size;
}
EXPORT_SYMBOL(ktf_symbol_size);

int ktf_kallsyms_init(void)
{
	char *ks;
#if (KERNEL_VERSION(5, 8, 0) < LINUX_VERSION_CODE)
	int stat = ktf_kallsyms_bootstrap();

	if (stat)
		return stat;
#else
	ki.kallsyms_lookup_name = kallsyms_lookup_name;
	ki.kallsyms_on_each_symbol = kallsyms_on_each_symbol;
#endif
	/* We rely on being able to resolve this symbol for looking up module
	 * specific internal symbols (multiple modules may define the same symbol):
	 */
	ks = "module_kallsyms_lookup_name";
	ki.module_kallsyms_lookup_name = (void *)ki.kallsyms_lookup_name(ks);
	if (!ki.module_kallsyms_lookup_name) {
		terr("Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
		     ks);
		return -EINVAL;
	}
	ks = "kallsyms_on_each_symbol";
	ki.kallsyms_on_each_symbol = (void *)ki.kallsyms_lookup_name(ks);
	if (!ki.kallsyms_on_each_symbol) {
		terr("Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
		     ks);
		return -EINVAL;
	}
	ks = "kallsyms_lookup_size_offset";
	ki.kallsyms_lookup_size_offset = (void *)ki.kallsyms_lookup_name(ks);
	if (!ki.kallsyms_lookup_size_offset) {
		terr("Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
		     ks);
		return -EINVAL;
	}
	return 0;
}
