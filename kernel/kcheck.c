#include <linux/module.h>
#include "kcheck.h"
#include <net/netlink.h>
#include <net/genetlink.h>

#include "nl.h"
#include "unlproto.h"
#include "ktest.h"

/* The array of test cases defined */
TCase check_test_case[MAX_TEST_CASES];

/* Number of elements in check_test_case[] */
int check_test_cnt = 0;

/* a mutex to protect this datastructure */
DEFINE_SPINLOCK(tc_lock);

#define MAX_PRINTF 4096

/* TBD: access to check_test_cases should be properly synchronized
 * to avoid crashes if someone tries to unload while a test in progress
 */

u32 assert_cnt = 0;

void flush_assert_cnt(struct sk_buff* skb)
{
	if (assert_cnt) {
		tlog(T_DEBUG, "update: %d asserts", assert_cnt);
		nla_put_u32(skb, KTEST_A_STAT, assert_cnt);
		assert_cnt = 0;
	}
}


long _fail_unless (struct sk_buff* skb, int result, const char *file,
			int line, const char *fmt, ...)
{
	int len;
	va_list ap;
	char* buf = "";
	if (result)
		assert_cnt++;
	else {
		flush_assert_cnt(skb);
		nla_put_u32(skb, KTEST_A_STAT, result);
		buf = (char*)kmalloc(MAX_PRINTF, GFP_KERNEL);
		nla_put_string(skb, KTEST_A_FILE, file);
		nla_put_u32(skb, KTEST_A_NUM, line);

		va_start(ap,fmt);
		len = vsnprintf(buf,MAX_PRINTF-1,fmt,ap);
		buf[len] = 0;
		va_end(ap);
		nla_put_string(skb, KTEST_A_STR, buf);
		tlog(T_ERROR, "file %s line %d: result %d (%s)",
			file, line, result, buf);
		kfree(buf);
	}
	return result;
}
EXPORT_SYMBOL(_fail_unless);


/* Add a test to a testcase:
 * Tests are represented by fun_hook objects that are linked into
 * two lists: fun_hook::flist in TCase::fun_list and
 *            fun_hook::hlist in ktest_handle::test_list
 */
void  _tcase_add_test (struct __test_desc td,
				struct ktest_handle *th,
				int _signal,
				int allowed_exit_value,
				int start, int end)
{
	TCase *tc;
	struct fun_hook *fc = (struct fun_hook *)
		kmalloc(sizeof(struct fun_hook), GFP_KERNEL);
	if (!fc)
		return;

	spin_lock(&tc_lock);
	tc = tcase_find(td.tclass);
	if (!tc) {
		printk(KERN_INFO "ERROR: Failed to add test %s from %s - no such test case \"%s\"",
			td.name, td.file, td.tclass);
		kfree(fc);
		goto out;
	}
	fc->name = td.name;
	fc->tclass = td.tclass;
	fc->fun = td.fun;
	fc->start = start;
	fc->end = end;
	fc->handle = th;

	DM(T_LIST, printk(KERN_INFO "ktest Testcase %s: Added test \"%s\""
		" start = %d, end = %d\n",
		td.tclass, td.name, start, end));
	list_add(&fc->flist, &tc->fun_list);
	list_add(&fc->hlist, &th->test_list);
out:
	spin_unlock(&tc_lock);
}
EXPORT_SYMBOL(_tcase_add_test);

/* Create a test case */
TCase*  tcase_create (const char *name)
{
	TCase* tc = NULL;
	spin_lock(&tc_lock);
	DM(T_INFO, printk(KERN_INFO "ktest: Added test set %s\n", name));
	if (check_test_cnt > MAX_TEST_CASES) {
		printk(KERN_ERR "ktest: Too many tests - increase MAX_TEST_CASES!");
		goto out;
	}
	tc = &check_test_case[check_test_cnt++];
	INIT_LIST_HEAD(&tc->fun_list);
	strncpy(tc->name, name, MAX_LEN_TEST_NAME);
out:
	spin_unlock(&tc_lock);
	return tc;
}

TCase*  tcase_find (const char *name)
{
	int i;
	for (i = 0; i < check_test_cnt; i++) {
		if (strcmp(name,check_test_case[i].name) == 0)
			return &check_test_case[i];
	}
	return NULL;
}


/* Clean up all tests associated with a ktest_handle */

void _tcase_cleanup(struct ktest_handle *th)
{
	struct fun_hook *fh;
	struct list_head *pos, *n;

	spin_lock(&tc_lock);
	list_for_each_safe(pos, n, &th->test_list) {
		fh = list_entry(pos, struct fun_hook, hlist);
		DM(T_INFO, printk(KERN_INFO "ktest: delete test %s\n", fh->name));
		list_del(&fh->flist);
		list_del(&fh->hlist);
		kfree(fh);
	}
	spin_unlock(&tc_lock);
}
EXPORT_SYMBOL(_tcase_cleanup);




void ktest_cleanup_check(void)
{
	int i;
	struct fun_hook *fh;
	struct list_head *pos;

	spin_lock(&tc_lock);
	for (i = 0; i < check_test_cnt; i++) {
		list_for_each(pos, &check_test_case[i].fun_list) {
			fh = list_entry(pos, struct fun_hook, flist);
			printk(KERN_INFO "ktest: test set %s still active at unload!\n", fh->name);
		}
	}
	spin_unlock(&tc_lock);
}
