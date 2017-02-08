/* Copyright (c) 2012 Oracle Corporation. All rights reserved */

#include "debug.h"
#include <stdlib.h>

unsigned long ktest_debug_mask = 0;


void ktest_debug_init()
{
  ktest_debug_mask = 0;
  char* dbg_mask_str = getenv("KTEST_DEBUG_MASK");
  if (dbg_mask_str) {
    ktest_debug_mask = strtol(dbg_mask_str, NULL, 0);
    log(KTEST_INFO_V, "debug mask set to 0x%lx\n", ktest_debug_mask);
  }
}
