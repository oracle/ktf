/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * nl.h: ktf netlink protocol interface
 */
#ifndef KTF_NL_H
#define KTF_NL_H

int ktf_nl_register(void);
void ktf_nl_unregister(void);

#endif
