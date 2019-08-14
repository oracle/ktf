// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_netctx.h: Configurable context setup for multinode network tests
 *
 * KTF implements handling on the kernel side for this but leaves
 * user space implementation to construct the corresponding
 * stuct ktf_addrinfo parameter block
 */

#ifndef _KTF_NETCTX_H
#define _KTF_NETCTX_H

/* KTF matches against this type id as a possible discriminator: */
#define KTF_NETCTX_TYPE_ID 0x2222

#define IFNAMSZ 16

struct ktf_peer_address
{
	struct sockaddr_storage addr; /* Address to use for this peer */
	char ifname[IFNAMSZ];	    /* Local name of the interface with this address at peer */
};

struct ktf_addrinfo
{
	short n;		    /* Number of nodes involved, including the local */
	short rank;		    /* Index into ktf_peer_address that corresponds to local host */
	struct ktf_peer_address a[2]; /* KTF expects size n instead of 2 here */
};

#ifdef __KERNEL__

struct ktf_netctx {
	struct ktf_context k;
	struct ktf_addrinfo *a; /* Addr.info dyn.allocated based on incoming data */
	size_t a_sz;		/* Size of the allocation in a, if any */
	short min_nodes;	/* Minimum number of nodes for this context */
	short max_nodes;	/* Maximum number of nodes this context supports */
};

struct ktf_netctx_type {
	struct ktf_context_type t;
	short min_nodes;	/* Minimum number of nodes for the context type */
	short max_nodes;	/* Maximum number of nodes for the context type */
};

int ktf_netctx_enable(struct ktf_handle *handle, struct ktf_netctx_type *nct,
		      short min_nodes, short max_nodes);

int ktf_netctx_cb(struct ktf_context *ctx, const void *data, size_t data_sz);
void ktf_netctx_cleanup(struct ktf_context *ctx);

struct sockaddr_storage *ktf_netctx_addr(struct ktf_netctx *ctx, short rank);
const char *ktf_netctx_ifname(struct ktf_netctx *ctx, short rank);
short ktf_netctx_rank(struct ktf_netctx *ctx);
short ktf_netctx_n(struct ktf_netctx *ctx);

#endif

#endif
