#include <linux/module.h>
#include "kcheck.h"
#include <net/netlink.h>
#include <net/genetlink.h>

#include "nl.h"
#include "unlproto.h"
#include "ktf.h"

/* The global map from name to ktf_case */
DEFINE_KTF_MAP(test_cases);

/* a lock to protect this datastructure */
DEFINE_MUTEX(tc_lock);

#define MAX_PRINTF 4096


/* Current total number of test cases defined */
size_t ktf_case_count()
{
	return ktf_map_size(&test_cases);
}

struct ktf_case *ktf_case_create(const char *name)
{
	struct ktf_case *tc = kmalloc(sizeof(*tc), GFP_KERNEL);
	int ret;

	if (!tc)
		return tc;

	INIT_LIST_HEAD(&tc->test_list);
	ret = ktf_map_elem_init(&tc->kmap, name);
	if (ret) {
		kfree(tc);
		return NULL;
	}
	DM(T_DEBUG, printk(KERN_INFO "ktf: Added test set %s\n", name));
	return tc;
}

struct ktf_case *ktf_case_find(const char *name)
{
	return ktf_map_find_entry(&test_cases, name, struct ktf_case, kmap);
}

struct ktf_case *ktf_case_find_create(const char *name)
{
	struct ktf_case *tc;
	int ret = 0;

	tc = ktf_case_find(name);
	if (!tc) {
		tc = ktf_case_create(name);
		ret = ktf_map_insert(&test_cases, &tc->kmap);
		if (ret) {
			kfree(tc);
			tc = NULL;
		}
	}
	return tc;
}

/* TBD: access to 'test_cases' should be properly synchronized
 * to avoid crashes if someone tries to unload while a test in progress
 */

u32 assert_cnt = 0;

void flush_assert_cnt(struct ktf_test *self)
{
	if (assert_cnt) {
		tlog(T_DEBUG, "update: %d asserts", assert_cnt);
		if (self->skb)
			nla_put_u32(self->skb, KTF_A_STAT, assert_cnt);
		assert_cnt = 0;
	}
}


long _fail_unless (struct ktf_test *self, int result, const char *file,
			int line, const char *fmt, ...)
{
	int len;
	va_list ap;
	char *buf;
	char bufprefix[KTF_MAX_NAME];

	if (result)
		assert_cnt++;
	else {
		flush_assert_cnt(self);
		buf = (char*)kmalloc(MAX_PRINTF, GFP_KERNEL);
		if (!buf)
			return result;
		va_start(ap,fmt);
		len = vsnprintf(buf,MAX_PRINTF-1,fmt,ap);
		buf[len] = 0;
		va_end(ap);
		if (self->skb) {
			nla_put_u32(self->skb, KTF_A_STAT, result);
			nla_put_string(self->skb, KTF_A_FILE, file);
			nla_put_u32(self->skb, KTF_A_NUM, line);
			nla_put_string(self->skb, KTF_A_STR, buf);
		}
		(void) snprintf(bufprefix, sizeof (bufprefix) - 1,
				"file %s line %d: result %d: ", file, line,
				result);
		tlog(T_ERROR, "%s%s", bufprefix, buf);
		(void) strncat(self->log, bufprefix, KTF_MAX_LOG);
		(void) strncat(self->log, buf, KTF_MAX_LOG);
		kfree(buf);
	}
	return result;
}
EXPORT_SYMBOL(_fail_unless);


/* Add a test to a testcase:
 * Tests are represented by ktf_test objects that are linked into
 * two lists: ktf_test::tlist in TCase::test_list and
 *            ktf_test::hlist in ktf_handle::test_list
 *
 * TCase::test_list is used for iterating through the tests.
 * ktf_handle::test_list is needed for cleanup.
 */
void  _tcase_add_test (struct __test_desc td,
				struct ktf_handle *th,
				int _signal,
				int allowed_exit_value,
				int start, int end)
{
	struct ktf_case *tc;
	struct ktf_test *t;
	char *log;

	log = kzalloc(KTF_MAX_LOG, GFP_KERNEL);
	if (!log)
		return;

	t = (struct ktf_test *)kzalloc(sizeof(struct ktf_test), GFP_KERNEL);
	if (!t) {
		kfree(log);
		return;
	}

	mutex_lock(&tc_lock);
	tc = ktf_case_find_create(td.tclass);
	if (!tc) {
		printk(KERN_INFO "ERROR: Failed to add test %s from %s to test case \"%s\"",
			td.name, td.file, td.tclass);
		mutex_unlock(&tc_lock);
		kfree(log);
		kfree(t);
		return;
	}
	t->name = td.name;
	t->tclass = td.tclass;
	t->fun = td.fun;
	t->start = start;
	t->end = end;
	t->handle = th;
	t->log = log;

	DM(T_LIST, printk(KERN_INFO "ktf: Added test \"%s.%s\""
		" start = %d, end = %d\n",
		td.tclass, td.name, start, end));
	list_add(&t->tlist, &tc->test_list);
	list_add(&t->hlist, &th->test_list);
	mutex_unlock(&tc_lock);
}
EXPORT_SYMBOL(_tcase_add_test);


/* Clean up all tests associated with a ktf_handle */

void _tcase_cleanup(struct ktf_handle *th)
{
	struct ktf_test *t;
	struct list_head *pos, *n;

	mutex_lock(&tc_lock);
	list_for_each_safe(pos, n, &th->test_list) {
		t = list_entry(pos, struct ktf_test, hlist);
		DM(T_LIST, printk(KERN_INFO "ktf: delete test %s.%s\n", t->tclass, t->name));
		list_del(&t->tlist);
		list_del(&t->hlist);
		kfree(t->log);
		kfree(t);
	}
	mutex_unlock(&tc_lock);
}
EXPORT_SYMBOL(_tcase_cleanup);




int ktf_cleanup(void)
{
	struct ktf_test *t;
	struct ktf_case *tc;

	mutex_lock(&tc_lock);
	ktf_map_for_each_entry(tc, &test_cases, kmap) {
		struct list_head *pos;
		list_for_each(pos, &tc->test_list) {
			t = list_entry(pos, struct ktf_test, tlist);
			printk(KERN_WARNING
				"ktf: (memory leak) test set %s still active with test %s at unload!\n",
				tc_name(tc), t->name);
			return -EBUSY;
		}
	}
	ktf_map_delete_all(&test_cases, struct ktf_case, kmap);
	mutex_unlock(&tc_lock);
	return 0;
}
