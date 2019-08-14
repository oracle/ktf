// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * unlproto.c: This file is needed because the C struct init
 * used in kernel/unlproto.h is not allowed in C++
 */

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#define NL_INTERNAL 1
#include "kernel/ktf_unlproto.h"


struct nla_policy *ktf_get_gnl_policy(void)
{
  return ktf_gnl_policy;
}
