#ifndef _KTEST_DEBUG_H
#define _KTEST_DEBUG_H
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

/* Debug function definitions - intended for test debugging
 * Enabled by setting bits in the environment variable
 *  KTEST_DEBUG_MASK
 */

extern unsigned long ktest_debug_mask;


#define KTEST_ERR             0x1
#define KTEST_WARN            0x2
#define KTEST_INFO            0x4
#define KTEST_INFO_V        0x100
#define KTEST_MR           0x2000
#define KTEST_DEBUG       0x10000
#define KTEST_POLL        0x20000
#define KTEST_EVENT       0x40000
#define KTEST_DEBUG_V   0x1000000
#define KTEST_DUMP      0x2000000

/* Call this to initialize the debug logic from
 * environment KTEST_DEBUG_MASK
 */
void ktest_debug_init();

#define log(level, format, arg...)		\
do {\
  if (level & ktest_debug_mask) {\
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
    if (ktest_debug_mask & class) { \
      stmt_list;  \
    }   \
  } while (0)

#endif
