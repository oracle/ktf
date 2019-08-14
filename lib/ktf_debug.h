// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012 Oracle Corporation. All rights reserved
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_debug.h: User mode debug function definitions
 * - intended for test debugging.
 *
 * Enabled by setting bits in the environment variable KTF_DEBUG_MASK
 */

#ifndef _KTF_DEBUG_H
#define _KTF_DEBUG_H
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

extern unsigned long ktf_debug_mask;


#define KTF_ERR             0x1
#define KTF_WARN            0x2
#define KTF_INFO            0x4
#define KTF_INFO_V        0x100
#define KTF_MR           0x2000
#define KTF_DEBUG       0x10000
#define KTF_POLL        0x20000
#define KTF_EVENT       0x40000
#define KTF_DEBUG_V   0x1000000
#define KTF_DUMP      0x2000000

/* Call this to initialize the debug logic from
 * environment KTF_DEBUG_MASK
 */
void ktf_debug_init();

#define log(level, format, arg...)		\
do {\
  if (level & ktf_debug_mask) {\
    char _tm[30]; \
    time_t _tv = time(NULL);\
    ctime_r(&_tv,_tm);\
    _tm[24] = '\0';\
    fprintf(stderr, "%s [%ld] %s: " format, \
            _tm, (long unsigned int) pthread_self(), __func__, ## arg);     \
  }\
} while (0)

#define logs(class, stmt_list) \
  do {							    \
    if (ktf_debug_mask & class) { \
      stmt_list;  \
    }   \
  } while (0)

#endif
