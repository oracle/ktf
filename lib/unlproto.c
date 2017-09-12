/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * unlproto.c: This file is needed because the C struct init
 * used in kernel/unlproto.h is not allowed in C++
 */

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#define NL_INTERNAL 1
#include "kernel/unlproto.h"


struct nla_policy *get_ktf_gnl_policy()
{
  return ktf_gnl_policy;
}
