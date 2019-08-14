// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (c) 2012 Oracle Corporation. All rights reserved
 *    Author: Knut Omang <knut.omang@oracle.com>
 */

#include "ktf_debug.h"
#include <stdlib.h>

unsigned long ktf_debug_mask = 0;


void ktf_debug_init()
{
  ktf_debug_mask = 0;
  char* dbg_mask_str = getenv("KTF_DEBUG_MASK");
  if (dbg_mask_str) {
    ktf_debug_mask = strtol(dbg_mask_str, NULL, 0);
    log(KTF_INFO_V, "debug mask set to 0x%lx\n", ktf_debug_mask);
  }
}
