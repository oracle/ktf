// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_netcfg.h: Configurable context setup for multinode network tests
 *
 */

#include "ktf.h"
#include "ktf_netctx.h"
#include <linux/in.h>

/* Configuration callback to configure a network context */

int ktf_netctx_cb(struct ktf_context *ctx, const void *data, size_t data_sz)
{
	struct ktf_netctx *nc = container_of(ctx, struct ktf_netctx, k);
	struct ktf_addrinfo *kai = (struct ktf_addrinfo *)data;
	short n = kai->n;
	size_t param_sz;

	if (n < 2) {
		terr("Unsupported number of nodes (%d) - must be at least 2", n);
		return -EINVAL;
	}

	param_sz = sizeof(*kai) + sizeof(kai->a) * (n - 2);

	if (n > nc->max_nodes || n < nc->min_nodes) {
		terr("Unsupported number of nodes (%d) - must be between %d and %d!",
		     n, nc->min_nodes, nc->max_nodes);
		return -EINVAL;
	}

	if (param_sz != data_sz) {
		terr("Expected %lu bytes of parameter data, received %lu!",
		     param_sz, data_sz);
		return -EINVAL;
	}

	if (nc->a && nc->a_sz != data_sz) {
		kfree(nc->a);
		nc->a = NULL;
	}

	if (!nc->a) {
		nc->a = kzalloc(data_sz, GFP_KERNEL);
		if (!nc->a)
			return -ENOMEM;
	}

	memcpy(nc->a, kai, data_sz);
	return 0;
}
EXPORT_SYMBOL(ktf_netctx_cb);

void ktf_netctx_cleanup(struct ktf_context *ctx)
{
	struct ktf_netctx *nc = container_of(ctx, struct ktf_netctx, k);

	kfree(nc->a);
}
EXPORT_SYMBOL(ktf_netctx_cleanup);

/* Make network contexts dynamically allocatable from user mode
 * Caller must supply desired values for callback functions in @nct.
 */
int ktf_netctx_enable(struct ktf_handle *handle, struct ktf_netctx_type *nct,
		      short min_nodes, short max_nodes)
{
	struct ktf_context *lo_ctx;
	struct ktf_addrinfo ai = {
		.n = 2,
		.rank = 0
	};
	int ret;
	int i;

	ret = ktf_handle_add_ctx_type(handle, &nct->t);
	if (ret)
		return ret;

	nct->min_nodes = min_nodes;
	nct->max_nodes = max_nodes;
	strcpy(nct->t.name, "netctx");

	for (i = 0; i < 2; i++) {
		struct sockaddr_in *ai_in = (struct sockaddr_in *)&ai.a[i].addr;

		ai.a[i].addr.ss_family = AF_INET;
		strcpy(ai.a[i].ifname, "lo");
		ai_in->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}

	/* create and configure the loopback network context */
	lo_ctx = ktf_context_add_from(handle, "lo", &nct->t);
	if (!lo_ctx)
		return -ENOMEM;

	ret = ktf_context_set_config(lo_ctx, &ai, sizeof(ai));
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL(ktf_netctx_enable);

struct sockaddr_storage *ktf_netctx_addr(struct ktf_netctx *ctx, short rank)
{
	return &ctx->a->a[rank].addr;
}
EXPORT_SYMBOL(ktf_netctx_addr);

const char *ktf_netctx_ifname(struct ktf_netctx *ctx, short rank)
{
	return ctx->a->a[rank].ifname;
}
EXPORT_SYMBOL(ktf_netctx_ifname);

short ktf_netctx_rank(struct ktf_netctx *ctx)
{
	return ctx->a->rank;
}
EXPORT_SYMBOL(ktf_netctx_rank);

short ktf_netctx_n(struct ktf_netctx *ctx)
{
	return ctx->a->n;
}
EXPORT_SYMBOL(ktf_netctx_n);
