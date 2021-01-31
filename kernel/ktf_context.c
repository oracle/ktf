// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 *
 * kft_context.c: Main part of ktf kernel module that implements a generic
 *   unit test framework for tests written in kernel code, with support for
 *   gtest (googletest) user space tools for invocation and reporting.
 */

#include <linux/module.h>
#include <rdma/ib_verbs.h>
#include "ktf.h"
#include "ktf_kallsyms.h"
#include "ktf_test.h"
#include "ktf_debugfs.h"
#include "ktf_nl.h"

MODULE_LICENSE("GPL");

ulong ktf_debug_mask = T_INFO | T_PRINTK;
EXPORT_SYMBOL(ktf_debug_mask);

static unsigned int ktf_context_maxid;

/* The role of context_lock is to synchronize modifications to
 * the global list of context handles (handles that have contexts
 * associated with them) and the context map.
 * The map object has it's own locking, but must be kept in sync
 * with changes to the global context list:
 */
static DEFINE_SPINLOCK(context_lock);

/* global linked list of all ktf_handle objects that have contexts */
LIST_HEAD(context_handles);

module_param_named(debug_mask, ktf_debug_mask, ulong, 0644);

static int __ktf_handle_add_ctx_type(struct ktf_handle *handle,
				     struct ktf_context_type *ct,
				     bool generic)
{
	unsigned long flags;
	int ret;

	if (generic && !(ct->alloc && ct->config_cb)) {
		terr("Mandatory configuration callbacks or values missing!");
		return -EINVAL;
	}

	ct->handle = handle;
	ktf_map_elem_init(&ct->elem, ct->name);

	spin_lock_irqsave(&context_lock, flags);
	ret = ktf_map_insert(&handle->ctx_type_map, &ct->elem);
	spin_unlock_irqrestore(&context_lock, flags);
	return ret;
}

static int __ktf_context_add(struct ktf_handle *handle, struct ktf_context *ctx,
			     const char *name, ktf_config_cb cfg_cb,
			     struct ktf_context_type *ct)
{
	unsigned long flags;
	int ret;

	ktf_map_elem_init(&ctx->elem, name);
	strncpy(ctx->name, name, KTF_MAX_NAME);
	ctx->config_cb = cfg_cb;
	ctx->config_errno = ENOENT; /* 0 here means configuration is ok */
	ctx->type = ct;
	ctx->cleanup = ct->cleanup;

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
	if (!ret)
		tlog(T_DEBUG, "added %scontext %s with type %s",
		     (cfg_cb ? "configurable " : ""), name, ct->name);
	return ret;
}

int ktf_context_add(struct ktf_handle *handle, struct ktf_context *ctx,
		    const char *name, ktf_config_cb cfg_cb,
		    const char *type_name)
{
	struct ktf_context_type *ct = ktf_handle_get_ctx_type(handle, type_name);
	int ret;

	if (!ct) {
		ct = kzalloc(sizeof(*ct), GFP_KERNEL);
		if (!ct)
			return -ENOMEM;
		strncpy(ct->name, type_name, KTF_MAX_KEY);
		ret = __ktf_handle_add_ctx_type(handle, ct, false);
		if (ret) {
			kfree(ct);
			return ret;
		}
	}
	return __ktf_context_add(handle, ctx, name, cfg_cb, ct);
}
EXPORT_SYMBOL(ktf_context_add);

struct ktf_context *ktf_context_add_from(struct ktf_handle *handle, const char *name,
					 struct ktf_context_type *ct)
{
	struct ktf_context *ctx;
	int ret;

	if (!ct->alloc) {
		terr("No alloc function supplied!");
		return NULL;
	}
	ctx = ct->alloc(ct);
	if (!ctx)
		return NULL;
	ret = __ktf_context_add(handle, ctx, name, ct->config_cb, ct);
	if (ret)
		goto fail;

	ctx->cleanup = ct->cleanup;
	return ctx;
fail:
	kfree(ctx);
	return NULL;
}
EXPORT_SYMBOL(ktf_context_add_from);

int ktf_context_set_config(struct ktf_context *ctx, const void *data, size_t data_sz)
{
	int ret;

	if (ctx->config_cb) {
		ret = ctx->config_cb(ctx, data, data_sz);
		ctx->config_errno = ret;
	}
	/* We don't use the map element refcounts for contexts, as
	 * the context objects may be allocated statically by client modules,
	 * just make sure the refcounts make sense from a debugging perspective:
	 */
	ktf_map_elem_put(&ctx->elem);
	return ctx->config_errno;
}
EXPORT_SYMBOL(ktf_context_set_config);

const char *ktf_context_name(struct ktf_context *ctx)
{
	return ctx->elem.key;
}
EXPORT_SYMBOL(ktf_context_name);

void ktf_context_remove(struct ktf_context *ctx)
{
	struct ktf_handle *handle;
	unsigned long flags = 0;

	if (!ctx) {
		terr("A test case tried to remove an invalid context!");
		return;
	}
	handle = ctx->handle;

	spin_lock_irqsave(&context_lock, flags);
	ktf_map_remove(&handle->ctx_map, ctx->elem.key);
	if (!ktf_has_contexts(handle))
		list_del(&handle->handle_list);
	spin_unlock_irqrestore(&context_lock, flags);

	tlog(T_DEBUG, "removed context %s at %p", ctx->elem.key, ctx);

	if (ctx->cleanup)
		ctx->cleanup(ctx);
	/* Note: ctx may be freed here! */
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
	if (!elem)
		return NULL;
	return container_of(elem, struct ktf_context, elem);
}
EXPORT_SYMBOL(ktf_find_context);

struct ktf_context *ktf_find_create_context(struct ktf_handle *handle, const char *name,
					    const char *type_name)
{
	struct ktf_context *ctx = ktf_find_context(handle, name);

	if (!ctx) {
		struct ktf_context_type *ct = ktf_handle_get_ctx_type(handle, type_name);

		tlog(T_DEBUG, "type = %s, ct = %p", type_name, ct);
		if (ct)
			ctx = ktf_context_add_from(handle, name, ct);
	}
	return ctx;
}

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
	struct ktf_context *curr;

	if (!ktf_has_contexts(handle))
		return;

	for (;;) {
		curr = ktf_find_first_context(handle);
		if (!curr)
			break;
		ktf_context_remove(curr);
	}
}
EXPORT_SYMBOL(ktf_context_remove_all);

/* Find the handle associated with handle id hid */
struct ktf_handle *ktf_handle_find(int hid)
{
	struct ktf_handle *handle = NULL;

	list_for_each_entry(handle, &context_handles, handle_list) {
		if (handle->id == hid)
			break;
	}
	return handle;
}

/* Allow user space to create new contexts of certain types
 * based on configuration types. This allocates a new, uniquely named
 * context type to enable it for user space usage. Caller must allocate and populate
 * @ct with appropriate callbacks and value for the context type.
 */

int ktf_handle_add_ctx_type(struct ktf_handle *handle,
			    struct ktf_context_type *ct)
{
	return __ktf_handle_add_ctx_type(handle, ct, true);
}
EXPORT_SYMBOL(ktf_handle_add_ctx_type);

struct ktf_context_type *ktf_handle_get_ctx_type(struct ktf_handle *handle,
						 const char *type_name)
{
	struct ktf_map_elem *elem = ktf_map_find(&handle->ctx_type_map, type_name);

	tlog(T_DEBUG, "Lookup %s in map size %lu = %p", type_name,
	     ktf_map_size(&handle->ctx_type_map), elem);
	if (!elem)
		return NULL;
	return container_of(elem, struct ktf_context_type, elem);
}

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

static int __init ktf_init(void)
{
	int ret;
	ktf_kallsyms_init();
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
	ktf_nl_unregister();
	ktf_cleanup();
}

/* Generic setup function for client modules */
void ktf_add_tests(ktf_test_adder f)
{
	f();
}
EXPORT_SYMBOL(ktf_add_tests);

module_init(ktf_init);
module_exit(ktf_exit);
