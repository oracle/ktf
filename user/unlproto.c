/* This file is needed because the C struct init
 * used is not allowed in C++
 */

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#define NL_INTERNAL 1
#include "kernel/unlproto.h"


struct nla_policy *get_ktest_gnl_policy()
{
  return ktest_gnl_policy;
}
