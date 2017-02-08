#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <unistd.h>
#include "kernel/unlproto.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "utest.h"
#include <map>
#include <set>
#include <string>
#include "debug.h"

#ifdef HAVE_LIBNL3
#include <netlink/version.h>
#else
#define nl_socket_alloc nl_handle_alloc
#define nl_socket_free nl_handle_destroy
#define nl_sock nl_handle
#endif


extern "C"
{
  /* From unlproto.c */
  struct nla_policy *get_ktest_gnl_policy();
}

int devcnt = 0;

namespace utest
{

struct nl_sock* sock = NULL;
int family = -1;

int printed_header = 0;

typedef std::map<std::string, KernelTest*> testmap;
typedef std::map<std::string, test_cb*> wrappermap;

class testset
{
public:
  testset() : setnum(0)
  { }

  ~testset()
  {
    for (testmap::iterator it = tests.begin(); it != tests.end(); ++it)
      delete it->second;
  }

  testmap tests;
  stringvec test_names;
  wrappermap wrapper;
  int setnum;
};

typedef std::map<std::string, testset> setmap;
typedef std::set<std::string> stringset;

struct name_iter
{
  setmap::iterator it;
  std::string setname;
};


/* We trick the gtest template framework
 * to get a new set of test names as a side effect of
 * invocation of get_test_names()
 */

/* Wrap globals in an object to control init order and
 * memory cleanup:
 */
class KernelTestMgr
{
public:
  KernelTestMgr() : next_set(0), cur(NULL)
  { }

  testset& find_add_set(std::string& setname);
  testset& find_add_test(std::string& setname, std::string& testname);
  void add_test(const std::string& setname, const char* tname);
  KernelTest* find_test(const std::string&setname, const std::string& testname);
  void add_wrapper(const std::string setname, const std::string testname, test_cb* tcb);

  stringvec& get_set_names() { return set_names; }
  stringvec get_test_names();

  stringvec get_testsets()
  {
    return set_names;
  }

  std::string get_current_setname()
  {
    return cur->setname;
  }

private:
  setmap sets;
  stringvec test_names;
  stringvec set_names;
  stringset kernelsets;
  int next_set;
  name_iter* cur;
};

KernelTestMgr& kmgr()
{
  static KernelTestMgr kmgr_;
  return kmgr_;
}

testset& KernelTestMgr::find_add_test(std::string& setname, std::string& testname)
{
  testset& ts(find_add_set(setname));
  test_names.push_back(testname);
  return ts;
}

testset& KernelTestMgr::find_add_set(std::string& setname)
{
  bool new_set = false;

  log(KTEST_INFO, "find_add_test(%s)\n", setname.c_str());

  stringset::iterator it = kernelsets.find(setname);
  if (it == kernelsets.end()) {
    kernelsets.insert(setname);
    set_names.push_back(setname);
    new_set = true;
  }

  testset& ts = sets[setname];
  if (new_set)
  {
    ts.setnum = next_set++;
    log(KTEST_INFO, "added %s (set %d)\n", setname.c_str(), ts.setnum);
  }
  return ts;
}


void KernelTestMgr::add_test(const std::string& setname, const char* tname)
{
  log(KTEST_DEBUG_V, "add_test: %s.%s\n", setname.c_str(),tname);
  std::string name(tname);
  new KernelTest(setname, tname);
}


KernelTest* KernelTestMgr::find_test(const std::string&setname, const std::string& testname)
{
  log(KTEST_DEBUG, "find test %s.%s\n", setname.c_str(), testname.c_str());
  return sets[setname].tests[testname];
}


/* Function for adding a wrapper user level test */
void KernelTestMgr::add_wrapper(const std::string setname, const std::string testname, test_cb* tcb)
{
  log(KTEST_DEBUG, "add_wrapper: %s.%s\n", setname.c_str(),testname.c_str());
  testset& ts = sets[setname];
  /* Depending on C++ initialization order which vary between compiler version
   * (sigh!) either the kernel tests have already been processed or we have to store
   * this object in wrapper for later insertion:
   */
  KernelTest *kt = ts.tests[testname];
  if (kt)
    kt->user_test = tcb;
  else
    ts.wrapper[testname] = tcb;
}

stringvec KernelTestMgr::get_test_names()
{
  if (!cur) {
    cur = new name_iter();
    cur->it = sets.begin();
  }

  if (cur->it == sets.end()) {
    delete cur;
    cur = NULL;
    return stringvec();
  }
  stringvec& v = cur->it->second.test_names;
  cur->setname = cur->it->first;
  ++(cur->it);
  return v;
}


KernelTest::KernelTest(const std::string& sn, const char* tn)
  : setname(sn),
    testname(tn),
    setnum(0),
    testnum(0),
    value(0),
    user_test(NULL),
    file(NULL),
    line(-1)
{

  name = setname;
  name.append(".");
  name.append(testname);

  testset& ts(kmgr().find_add_test(setname, testname));
  setnum = ts.setnum;
  ts.tests[testname] = this;
  ts.test_names.push_back(testname);
  testnum = ts.test_names.size();

  wrappermap::iterator hit = ts.wrapper.find(testname);
  if (hit != ts.wrapper.end())
    user_test = hit->second;
}



static int parse_cb(struct nl_msg *msg, void *arg);
static int debug_cb(struct nl_msg *msg, void *arg);
static int error_cb(struct nl_msg *msg, void *arg);

int nl_connect(void)
{
  /* Allocate a new netlink socket */
  sock = nl_socket_alloc();
  if (sock == NULL){
    fprintf(stderr, "Failed to allocate a nl socket");
    exit(1);
  }

  /* Connect to generic netlink socket on kernel side */
  int stat = genl_connect(sock);
  if (stat) {
    fprintf(stderr, "Failed to open generic netlink connection");
    exit(1);
  }

  /* Ask kernel to resolve family name to family id */
  family = genl_ctrl_resolve(sock, "ktest");
  if (family <= 0) {
    fprintf(stderr, "Netlink protocol family not found - is test driver loaded?\n");
    exit(1);
  }

  /* Specify the generic callback functions for messages */
  nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);
  nl_socket_modify_cb(sock, NL_CB_INVALID, NL_CB_CUSTOM, error_cb, NULL);
  return 0;
}


void default_test_handler(int result,  const char* file, int line, const char* report)
{
  if (result >= 0) {
    fprintf(stderr, "default_test_handler: Result %d: %s,%d\n",result,file,line);
  } else {
    fprintf(stderr, "default_test_handler: Result %d\n",result);
  }
}

test_handler handle_test = default_test_handler;

bool setup(test_handler ht)
{
  ktest_debug_init();
  handle_test = ht;
  return nl_connect() == 0;
}


/* Query kernel for available tests in index order */
stringvec& query_testsets()
{
  struct nl_msg *msg;
  int err;

  msg = nlmsg_alloc();
  genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST,
	      KTEST_C_REQ, 1);
  nla_put_u32(msg, KTEST_A_TYPE, KTEST_CT_QUERY);

  // Send message over netlink socket
  nl_send_auto_complete(sock, msg);

  // Free message
  nlmsg_free(msg);

  // Wait for acknowledgement:
  // This function also returns error status if the message
  // was not deemed ok by the kernel.
  //
  err = nl_wait_for_ack(sock);
  if (err < 0) {
    errno = -err;
    return kmgr().get_set_names();
  }

  // Then wait for the answer and receive it
  nl_recvmsgs_default(sock);
  return kmgr().get_set_names();
}

stringvec get_test_names()
{
  return kmgr().get_test_names();
}

std::string get_current_setname()
{
  return kmgr().get_current_setname();
}

KernelTest* find_test(const std::string&setname, const std::string& testname)
{
  return kmgr().find_test(setname, testname);
}

void add_wrapper(const std::string setname, const std::string testname, test_cb* tcb)
{
  kmgr().add_wrapper(setname, testname, tcb);
}

void run_test(KernelTest* kt)
{
  if (kt->user_test)
    kt->user_test->fun(kt);
  else
    run_kernel_test(kt);
}


void run_kernel_test(KernelTest* kt)
{
  struct nl_msg *msg;

  log(KTEST_DEBUG_V, "START kernel test (%ld,%ld): %s\n", kt->setnum,
		kt->testnum, kt->name.c_str());

  msg = nlmsg_alloc();
  genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST,
	      KTEST_C_REQ, 1);
  nla_put_u32(msg, KTEST_A_TYPE, KTEST_CT_RUN);
  nla_put_u32(msg, KTEST_A_SN, kt->setnum);
  nla_put_u32(msg, KTEST_A_NUM, kt->testnum);
  nla_put_u32(msg, KTEST_A_DEVNO, 0); /* Run only on device 0 right now */
  if (kt->value)
    nla_put_u32(msg, KTEST_A_STAT, kt->value);

  // Send message over netlink socket
  nl_send_auto_complete(sock, msg);

  // Free message
  nlmsg_free(msg);

  // Wait for acknowledgement - otherwise
  // nl_recvmsg_default will sometimes take the ack for the next message..
  int err = nl_wait_for_ack(sock);
  if (err < 0) {
    errno = -err;
    return;
  }

  // Wait for the answer and receive it
  nl_recvmsgs_default(sock);

  log(KTEST_DEBUG_V, "END   utest::run_kernel_test %s\n", kt->name.c_str());
}



static nl_cb_action parse_one_test(std::string& setname,
				   std::string& testname, struct nlattr* attr)
{
  int rem = 0;
  struct nlattr *nla;
  const char* msg;
  nla_for_each_nested(nla, attr, rem) {
    switch (nla->nla_type) {
    case KTEST_A_STR:
      msg = nla_get_string(nla);
      kmgr().add_test(setname,msg);
      break;
    default:
      fprintf(stderr,"parse_result: Unexpected attribute type %d\n", nla->nla_type);
      return NL_SKIP;
    }
  }
  return NL_OK;
}



static int parse_query(struct nl_msg *msg, struct nlattr** attrs)
{
  int alloc = 0, rem = 0;
  nl_cb_action stat;
  std::string setname,testname;

  if (attrs[KTEST_A_NUM]) {
    alloc = nla_get_u32(attrs[KTEST_A_NUM]);
    log(KTEST_DEBUG, "Kernel offers %d test sets:\n", alloc);
  } else {
    fprintf(stderr,"No test set count in kernel response??\n");
    return -1;
  }

  if (attrs[KTEST_A_DEVNO]) {
    devcnt = nla_get_u32(attrs[KTEST_A_NUM]);
    log(KTEST_INFO_V, "A total of %d devices registered.\n", devcnt);
  }

  if (attrs[KTEST_A_LIST]) {
    struct nlattr *nla;

    nla_for_each_nested(nla, attrs[KTEST_A_LIST], rem) {
      switch (nla->nla_type) {
      case KTEST_A_STR:
	setname = nla_get_string(nla);
	break;
      case KTEST_A_TEST:
	stat = parse_one_test(setname, testname, nla);
	if (stat != NL_OK)
	  return stat;
	break;
      default:
	fprintf(stderr,"parse_result: Unexpected attribute type %d\n", nla->nla_type);
	return NL_SKIP;
      }
      kmgr().find_add_set(setname); /* Just to make sure empty sets are also added */
    }
  }

  return NL_OK;
}


static enum nl_cb_action parse_result(struct nl_msg *msg, struct nlattr** attrs)
{
  int assert_cnt = 0, fail_cnt = 0;
  int rem = 0, testnum;
  const char *file = "no_file",*report = "no_report";

  if (attrs[KTEST_A_NUM]) {
    testnum = nla_get_u32(attrs[KTEST_A_NUM]);
    log(KTEST_DEBUG, "parsed test number %d\n",testnum);
  }
  if (attrs[KTEST_A_LIST]) {
    /* Parse list of test results */
    struct nlattr *nla;
    int result = -1, line = 0;
    nla_for_each_nested(nla, attrs[KTEST_A_LIST], rem) {
      switch (nla->nla_type) {
      case KTEST_A_STAT:
	/* Flush previous test, if any */
	handle_test(result,file,line,report);
	result = nla_get_u32(nla);
	/* Our own count and report since check does such a lousy
	 * job in counting individual checks */
	if (result)
	  assert_cnt += result;
	else {
	  fail_cnt++;
	  assert_cnt++;
	}
	break;
      case KTEST_A_FILE:
	file = nla_get_string(nla);
	if (!file)
	  file = "no_file";
	break;
      case KTEST_A_NUM:
	line = nla_get_u32(nla);
	break;
      case KTEST_A_STR:
	report = nla_get_string(nla);
	if (!report)
	  report = "no_report";
	break;
      default:
	fprintf(stderr,"parse_result: Unexpected attribute type %d\n", nla->nla_type);
	return NL_SKIP;
      }
    }
    /* Handle last test */
    handle_test(result,file,line,report);
  }

  return NL_OK;
}


static int parse_cb(struct nl_msg *msg, void *arg)
{
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  int maxtype = KTEST_A_MAX+10;
  struct nlattr *attrs[maxtype];
  enum ktest_cmd_type type;

  //  memset(attrs, 0, sizeof(attrs));

  /* Validate message and parse attributes */
  int err = genlmsg_parse(nlh, 0, attrs, KTEST_A_MAX, get_ktest_gnl_policy());
  if (err < 0) return err;

  if (!attrs[KTEST_A_TYPE]) {
    fprintf(stderr, "Received kernel response without a type\n");
    return NL_SKIP;
  }

  type = (ktest_cmd_type)nla_get_u32(attrs[KTEST_A_TYPE]);
  switch (type) {
  case KTEST_CT_QUERY:
    return parse_query(msg, attrs);
  case KTEST_CT_RUN:
    return parse_result(msg, attrs);
  default:
    debug_cb(msg, attrs);
  }
  return NL_SKIP;
}


static int error_cb(struct nl_msg *msg, void *arg)
{
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  fprintf(stderr, "Received invalid netlink message - type %d\n", nlh->nlmsg_type);
  return NL_OK;
}


static int debug_cb(struct nl_msg *msg, void *arg)
{
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  fprintf(stderr, "[Received netlink message of type %d]\n", nlh->nlmsg_type);
    nl_msg_dump(msg, stderr);
    return NL_OK;
}

} // end namespace utest
