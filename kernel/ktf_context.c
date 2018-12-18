/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * kft_context.c: Main part of ktf kernel module that implements a generic
 *   unit test framework for tests written in kernel code, with support for
 *   gtest (googletest) user space tools for invocation and reporting.
 */

#include <linux/module.h>
#include <linux/kallsyms.h>
#include <rdma/ib_verbs.h>
#include "ktf.h"
#include "ktf_test.h"
#include "ktf_debugfs.h"
#include "ktf_nl.h"

MODULE_LICENSE("GPL");

ulong ktf_debug_mask = T_INFO;
EXPORT_SYMBOL(ktf_debug_mask);

static unsigned int ktf_context_maxid;

static DEFINE_SPINLOCK(context_lock);

/* global linked list of all ktf_handle objects that have contexts */
LIST_HEAD(context_handles);

module_param_named(debug_mask, ktf_debug_mask, ulong, 0644);

int ktf_context_add(struct ktf_handle *handle, struct ktf_context *ctx, const char *name)
{
	unsigned long flags;
	int ret;

	tlog(T_DEBUG, "added context %s (at %p)", name, ctx);
	ktf_map_elem_init(&ctx->elem, name);

	spin_lock_irqsave(&context_lock, flags);
	ret = ktf_map_insert(&handle->ctx_map, &ctx->elem);
	if (!ret) {
		ctx->handle = handle;
		if (ktf_map_size(&handle->ctx_map) == 1) {
			handle->id = ++ktf_context_maxid;
			INIT_LIST_HEAD(&handle->handle_list);
			list_add(&handle->handle_list, &context_handles);
		}
	}
	spin_unlock_irqrestore(&context_lock, flags);
	return ret;
}
EXPORT_SYMBOL(ktf_context_add);

const char *ktf_context_name(struct ktf_context *ctx)
{
	return ctx->elem.key;
}
EXPORT_SYMBOL(ktf_context_name);

static void __ktf_context_remove(struct ktf_context *ctx, bool locked)
{
	struct ktf_handle *handle;
	unsigned long flags = 0;

	if (!ctx) {
		terr("A test case tried to remove an invalid context!");
		return;
	}
	handle = ctx->handle;

	/* ktf_find_context might be called from interrupt level */
	if (!locked)
		spin_lock_irqsave(&context_lock, flags);

	ktf_map_remove(&handle->ctx_map, ctx->elem.key);
	if (!ktf_has_contexts(handle))
		list_del(&handle->handle_list);

	if (!locked)
		spin_unlock_irqrestore(&context_lock, flags);
	tlog(T_DEBUG, "removed context %s at %p", ctx->elem.key, ctx);
}

void ktf_context_remove(struct ktf_context *ctx)
{
	__ktf_context_remove(ctx, false);
}
EXPORT_SYMBOL(ktf_context_remove);

struct ktf_context *ktf_find_first_context(struct ktf_handle *handle)
{
	struct ktf_map_elem *elem = ktf_map_find_first(&handle->ctx_map);

	if (elem)
		return container_of(elem, struct ktf_context, elem);
	return NULL;
}

struct ktf_context *ktf_find_context(struct ktf_handle *handle, const char *name)
{
	struct ktf_map_elem *elem;

	if (!name)
		return NULL;
	elem = ktf_map_find(&handle->ctx_map, name);
	return container_of(elem, struct ktf_context, elem);
}
EXPORT_SYMBOL(ktf_find_context);

struct ktf_context *ktf_find_next_context(struct ktf_context *ctx)
{
	struct ktf_map_elem *elem = ktf_map_find_next(&ctx->elem);

	return container_of(elem, struct ktf_context, elem);
}

size_t ktf_has_contexts(struct ktf_handle *handle)
{
	return ktf_map_size(&handle->ctx_map) > 0;
}
EXPORT_SYMBOL(ktf_has_contexts);

void ktf_context_remove_all(struct ktf_handle *handle)
{
	struct ktf_context *curr, *next;
	unsigned long flags;

	if (!ktf_has_contexts(handle))
		return;

	spin_lock_irqsave(&context_lock, flags);

	curr = ktf_find_first_context(handle);

	while (curr) {
		next = ktf_find_next_context(curr);
		__ktf_context_remove(curr, true);
		curr = next;
	}
	spin_unlock_irqrestore(&context_lock, flags);
}
EXPORT_SYMBOL(ktf_context_remove_all);

void ktf_handle_cleanup_check(struct ktf_handle *handle)
{
	struct ktf_context *curr;
	unsigned long flags;

	if (!ktf_has_contexts(handle))
		return;

	spin_lock_irqsave(&context_lock, flags);

	for (curr = ktf_find_first_context(handle);
	     curr;
	     curr = ktf_find_next_context(curr)) {
		twarn("context %s found during handle %p cleanup", curr->elem.key, handle);
	}
	spin_unlock_irqrestore(&context_lock, flags);
}
EXPORT_SYMBOL(ktf_handle_cleanup_check);

struct ktf_kernel_internals {
	/* From module.h: Look up a module symbol - supports syntax module:name */
	unsigned long (*module_kallsyms_lookup_name)(const char *name);
	/* From kallsyms.h: Look up a symbol w/size and offset */
	unsigned long (*kallsyms_lookup_size_offset)(unsigned long addr,
						     unsigned long *symbolsize,
						     unsigned long *offset);
};

static struct ktf_kernel_internals ki;

static int __init ktf_init(void)
{
	int ret;
	char *ks = "module_kallsyms_lookup_name";

	/* We rely on being able to resolve this symbol for looking up module
	 * specific internal symbols (multiple modules may define the same symbol):
	 */
	ki.module_kallsyms_lookup_name = (void *)kallsyms_lookup_name(ks);
	if (!ki.module_kallsyms_lookup_name) {
		terr("Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
		     ks);
		return -EINVAL;
	}
	ks = "kallsyms_lookup_size_offset";
	ki.kallsyms_lookup_size_offset = (void *)kallsyms_lookup_name(ks);
	if (!ki.kallsyms_lookup_size_offset) {
		terr("Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
		     ks);
		return -EINVAL;
	}

	ktf_debugfs_init();
	ret = ktf_nl_register();
	if (ret) {
		terr("Unable to register protocol with netlink");
		ktf_debugfs_cleanup();
		goto failure;
	}

	return 0;
failure:
	return ret;
}

static void __exit ktf_exit(void)
{
	ktf_cleanup();
	ktf_nl_unregister();
}

/* Generic setup function for client modules */
void ktf_add_tests(ktf_test_adder f)
{
	f();
}
EXPORT_SYMBOL(ktf_add_tests);

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
		addr = kallsyms_lookup_name(symref);
	}
	if (!addr)
		addr = ki.module_kallsyms_lookup_name(symref);
	if (addr) {
		tlog(T_DEBUG, "Found %s at %0lx\n", sym, addr);
	} else {
#ifndef CONFIG_KALLSYMS_ALL
		twarn("CONFIG_KALLSYMS_ALL is not set, so non-exported symbols are not available\n");
#endif
		tlog(T_INFO, "Fatal error: %s not found\n", sym);
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

module_init(ktf_init);
module_exit(ktf_exit);
