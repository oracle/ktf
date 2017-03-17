/*
 * Copyright (c) 2009-2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * kbase.c: Main part of ktf kernel module that implements a generic unit test
 *   framework for tests written in kernel code, with support for gtest
 *   (googletest) user space tools for invocation and reporting.
 *
 */

#include <linux/module.h>
#include <linux/kallsyms.h>
#include <rdma/ib_verbs.h>
#include "ktf.h"
#include "nl.h"

MODULE_LICENSE("GPL");

ulong ktf_debug_mask = T_INFO | T_ERROR;

unsigned int ktf_context_maxid = 0;

DEFINE_SPINLOCK(context_lock);

/* global linked list of all ktf_handle objects that have contexts */
LIST_HEAD(context_handles);

module_param_named(debug_mask, ktf_debug_mask, ulong, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL(ktf_debug_mask);

/* Defined in kcheck.c */
void ktf_cleanup_check(void);

int ktf_context_add(struct ktf_handle *handle, struct ktf_context *ctx, const char *name)
{
	unsigned long flags;
	int ret;

	printk ("ktf: added context %s (at %p)\n", name, ctx);
	ktf_map_elem_init(&ctx->elem, name);

	spin_lock_irqsave(&context_lock, flags);
	ret = ktf_map_insert(&handle->ctx_map, &ctx->elem);
	if (!ret) {
		ctx->handle = handle;
		if (ktf_map_size(&handle->ctx_map) == 1) {
			handle->id = ++ktf_context_maxid;
			list_add(&handle->handle_list, &context_handles);
		}
	}
	spin_unlock_irqrestore(&context_lock, flags);
	return ret;
}
EXPORT_SYMBOL(ktf_context_add);


void ktf_context_remove(struct ktf_context *ctx)
{
	unsigned long flags;
	struct ktf_handle *handle;

	if (!ctx) {
		tlog(T_ERROR, "A test case tried to remove an invalid context!");
		return;
	}
	handle = ctx->handle;

	/* ktf_find_context might be called from interrupt level */
	spin_lock_irqsave(&context_lock,flags);
	ktf_map_remove(&handle->ctx_map, ctx->elem.name);

	if (!ktf_has_contexts(handle))
		list_del(&handle->handle_list);
	spin_unlock_irqrestore(&context_lock,flags);
	printk ("ktf: removed context %s at %p\n", ctx->elem.name, ctx);
}
EXPORT_SYMBOL(ktf_context_remove);

struct ktf_context *ktf_find_first_context(struct ktf_handle *handle)
{
	struct ktf_map_elem *elem = ktf_map_find_first(&handle->ctx_map);
	if (elem)
		return container_of(elem, struct ktf_context, elem);
	return NULL;
}

struct ktf_context* ktf_find_context(struct ktf_handle *handle, const char* name)
{
	struct ktf_map_elem *elem;
	if (!name)
		return NULL;
	elem = ktf_map_find(&handle->ctx_map, name);
	return container_of(elem, struct ktf_context, elem);
}
EXPORT_SYMBOL(ktf_find_context);

struct ktf_context *ktf_find_next_context(struct ktf_context* ctx)
{
	struct ktf_map_elem *elem = ktf_map_find_next(&ctx->elem);
	return container_of(elem, struct ktf_context, elem);
}

struct ktf_kernel_internals {
	/* From module.h: Look up a module symbol - supports syntax module:name */
	unsigned long (*module_kallsyms_lookup_name)(const char *);
};

static struct ktf_kernel_internals ki;


static int __init ktf_init(void)
{
	int ret;
	const char* ks = "module_kallsyms_lookup_name";

	/* We rely on being able to resolve this symbol for looking up module
	 * specific internal symbols (multiple modules may define the same symbol):
	 */
	ki.module_kallsyms_lookup_name = (void*)kallsyms_lookup_name(ks);
	if (!ki.module_kallsyms_lookup_name) {
		printk(KERN_ERR "Unable to look up \"%s\" in kallsyms - maybe interface has changed?",
			ks);
		return -EINVAL;
	}

	ret = ktf_nl_register();
	if (ret) {
		printk(KERN_ERR "Unable to register protocol with netlink");
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


/* Support for looking up module internal symbols to enable testing */
void* ktf_find_symbol(const char *mod, const char *sym)
{
	char sm[200];
	const char *symref;
	unsigned long addr;

	if (mod) {
		sprintf(sm, "%s:%s", mod, sym);
		symref = sm;
	} else
		symref = sym;

	addr = ki.module_kallsyms_lookup_name(symref);
	if (addr)
		tlog(T_DEBUG, "Found %s at %0lx\n", sym, addr);
	else {
		tlog(T_INFO, "Fatal error: %s not found\n", sym);
		return NULL;
	}
	return (void*)addr;
}
EXPORT_SYMBOL(ktf_find_symbol);


module_init(ktf_init);
module_exit(ktf_exit);
