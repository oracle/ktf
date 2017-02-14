/*
 * Copyright (c) 2009-2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * kbase.c: Main part of ktest kernel module that implements a generic unit test
 *   framework for tests written in kernel code, with support for gtest
 *   (googletest) user space tools for invocation and reporting.
 *
 */

#include <linux/module.h>
#include <linux/kallsyms.h>
#include <rdma/ib_verbs.h>
#include "ktest.h"
#include "nl.h"

MODULE_LICENSE("GPL");

ulong ktest_debug_mask = T_INFO | T_ERROR;

unsigned int ktest_context_maxid = 0;

DEFINE_SPINLOCK(context_lock);

/* global linked list of all ktest_handle objects that have contexts */
LIST_HEAD(context_handles);

module_param_named(debug_mask, ktest_debug_mask, ulong, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL(ktest_debug_mask);

/* Defined in kcheck.c */
void ktest_cleanup_check(void);

int ktest_context_add(struct ktest_handle *handle, struct ktest_context *ctx, const char *name)
{
	unsigned long flags;
	int ret;

	printk ("ktest: added context %s (at %p)\n", name, ctx);
	ktest_map_elem_init(&ctx->elem, name);

	spin_lock_irqsave(&context_lock, flags);
	ret = ktest_map_insert(&handle->ctx_map, &ctx->elem);
	if (!ret) {
		ctx->handle = handle;
		if (ktest_map_size(&handle->ctx_map) == 1) {
			handle->id = ++ktest_context_maxid;
			list_add(&handle->handle_list, &context_handles);
		}
	}
	spin_unlock_irqrestore(&context_lock, flags);
	return ret;
}
EXPORT_SYMBOL(ktest_context_add);


void ktest_context_remove(struct ktest_context *ctx)
{
	unsigned long flags;
	struct ktest_handle *handle = ctx->handle;

	/* ktest_find_context might be called from interrupt level */
	spin_lock_irqsave(&context_lock,flags);
	ktest_map_remove(&handle->ctx_map, ctx->elem.name);

	if (!ktest_has_contexts(handle))
		list_del(&handle->handle_list);
	spin_unlock_irqrestore(&context_lock,flags);
	printk ("ktest: removed context %s at %p\n", ctx->elem.name, ctx);
}
EXPORT_SYMBOL(ktest_context_remove);

struct ktest_context *ktest_find_first_context(struct ktest_handle *handle)
{
	struct ktest_map_elem *elem = ktest_map_find_first(&handle->ctx_map);
	if (elem)
		return container_of(elem, struct ktest_context, elem);
	return NULL;
}

struct ktest_context* ktest_find_context(struct ktest_handle *handle, const char* name)
{
	struct ktest_map_elem *elem;
	if (!name)
		return NULL;
	elem = ktest_map_find(&handle->ctx_map, name);
	return container_of(elem, struct ktest_context, elem);
}
EXPORT_SYMBOL(ktest_find_context);

struct ktest_context *ktest_find_next_context(struct ktest_context* ctx)
{
	struct ktest_map_elem *elem = ktest_map_find_next(&ctx->elem);
	return container_of(elem, struct ktest_context, elem);
}

struct ktest_kernel_internals {
	/* From module.h: Look up a module symbol - supports syntax module:name */
	unsigned long (*module_kallsyms_lookup_name)(const char *);
};

static struct ktest_kernel_internals ki;


static int __init ktest_init(void)
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

	ret = ktest_nl_register();
	if (ret) {
		printk(KERN_ERR "Unable to register protocol with netlink");
		goto failure;
	}

	/* NB! Test classes must be inserted alphabetically */
	tcase_create("any");
	tcase_create("mlx");
	tcase_create("prm");
	tcase_create("rtl");

	/* long tests not part of checkin regression */
	tcase_create("rtlx");

	tcase_create("sif");
	return 0;
failure:
	return ret;
}


static void __exit ktest_exit(void)
{
	ktest_cleanup_check();
	ktest_nl_unregister();
}


/* Generic setup function for client modules */
void ktest_add_tests(ktest_test_adder f)
{
	f();
}
EXPORT_SYMBOL(ktest_add_tests);


/* Support for looking up module internal symbols to enable testing */
void* ktest_find_symbol(const char *mod, const char *sym)
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
EXPORT_SYMBOL(ktest_find_symbol);


module_init(ktest_init);
module_exit(ktest_exit);
