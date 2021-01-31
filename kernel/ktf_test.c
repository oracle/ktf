// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_test.c: Kernel side code for tracking and reporting ktf test results
 */
#include <linux/module.h>
#include <linux/timekeeping.h>
#include "ktf_test.h"
#include <net/netlink.h>
#include <net/genetlink.h>

#include "ktf_nl.h"
#include "ktf_unlproto.h"
#include "ktf.h"
#include "ktf_cov.h"
#include "ktf_debugfs.h"

#define MAX_PRINTF 4096

/* Versioning check:
 * For MAJOR or MINOR changes, both sides are required to
 * have the same version.
 * If MICRO has changed, some new functionality may have been added, but the
 * old functionality should work as before.
 * With only BUILD changes, the two versions are still compatible,
 * but one might have bug fixes or minor enhancements.
 */
int ktf_version_check(u64 version)
{
	if (version != KTF_VERSION_LATEST) {
		if (KTF_VERSION(MAJOR, version) == KTF_VERSION(MAJOR, KTF_VERSION_LATEST) &&
		    KTF_VERSION(MINOR, version) == KTF_VERSION(MINOR, KTF_VERSION_LATEST))
			return 0;
		terr("KTF version mismatch - expected %llu.%llu.%llu.%llu, got %llu.%llu.%llu.%llu",
		     KTF_VERSION(MAJOR, KTF_VERSION_LATEST),
		     KTF_VERSION(MINOR, KTF_VERSION_LATEST),
		     KTF_VERSION(MICRO, KTF_VERSION_LATEST),
		     KTF_VERSION(BUILD, KTF_VERSION_LATEST),
		     KTF_VERSION(MAJOR, version),
		     KTF_VERSION(MINOR, version),
		     KTF_VERSION(MICRO, version),
		     KTF_VERSION(BUILD, version));
		return -EINVAL;
	}
	return 0;
}

static int ktf_handle_version_check(struct ktf_handle *th)
{
	return ktf_version_check(th->version);
}

/* Function called when global references to test case reach 0. */
static void ktf_case_free(struct ktf_map_elem *elem)
{
	struct ktf_case *tc = container_of(elem, struct ktf_case, kmap);

	kfree(tc);
}

void ktf_case_get(struct ktf_case *tc)
{
	ktf_map_elem_get(&tc->kmap);
}

void ktf_case_put(struct ktf_case *tc)
{
	ktf_map_elem_put(&tc->kmap);
}

/* The global map from name to ktf_case */
DEFINE_KTF_MAP(test_cases, NULL, ktf_case_free);

/* a lock to protect this datastructure */
static DEFINE_MUTEX(tc_lock);

/* Current total number of test cases defined */
size_t ktf_case_count(void)
{
	return ktf_map_size(&test_cases);
}

const char *ktf_case_name(struct ktf_case *tc)
{
	return tc->kmap.key;
}

static size_t ktf_case_test_count(struct ktf_case *tc)
{
	return ktf_map_size(&tc->tests);
}

/* Called when test refcount reaches 0. */
static void ktf_test_free(struct ktf_map_elem *elem)
{
	struct ktf_test *t = container_of(elem, struct ktf_test, kmap);

	kfree(t->log);
	kfree(t);
}

void ktf_test_get(struct ktf_test *t)
{
	ktf_map_elem_get(&t->kmap);
}

void ktf_test_put(struct ktf_test *t)
{
	ktf_map_elem_put(&t->kmap);
}

static struct ktf_case *ktf_case_create(const char *name)
{
	struct ktf_case *tc = kmalloc(sizeof(*tc), GFP_KERNEL);
	int ret;

	if (!tc)
		return tc;

	/* Initialize test case map of tests. */
	ktf_map_init(&tc->tests, NULL, ktf_test_free);
	ret = ktf_map_elem_init(&tc->kmap, name);
	if (ret) {
		kfree(tc);
		return NULL;
	}
	ktf_debugfs_create_testset(tc);
	tlog(T_DEBUG, "ktf: Added test set %s", name);
	return tc;
}

struct ktf_case *ktf_case_find(const char *name)
{
	return ktf_map_find_entry(&test_cases, name, struct ktf_case, kmap);
}

/* Returns with case refcount increased.  Called with tc_lock held. */
static struct ktf_case *ktf_case_find_create(const char *name)
{
	struct ktf_case *tc;
	int ret = 0;

	tc = ktf_case_find(name);
	if (!tc) {
		tc = ktf_case_create(name);
		if (tc) {
			ret = ktf_map_insert(&test_cases, &tc->kmap);
			if (ret) {
				kfree(tc);
				tc = NULL;
			}
		}
	}
	return tc;
}

static atomic_t assert_cnt = ATOMIC_INIT(0);

void flush_assert_cnt(struct ktf_test *self)
{
	if (atomic_read(&assert_cnt)) {
		tlog(T_DEBUG, "update: %d asserts", atomic_read(&assert_cnt));
		if (self->skb)
			nla_put_u32(self->skb, KTF_A_STAT, atomic_read(&assert_cnt));
		atomic_set(&assert_cnt, 0);
	}
}

u32 ktf_get_assertion_count(void)
{
	return atomic_read(&assert_cnt);
}
EXPORT_SYMBOL(ktf_get_assertion_count);

static DEFINE_SPINLOCK(assert_lock);

long _ktf_assert(struct ktf_test *self, int result, const char *file,
		 int line, const char *fmt, ...)
{
	int len;
	va_list ap;
	char *buf;
	char bufprefix[256];
	unsigned long flags;

	if (result) {
		atomic_inc(&assert_cnt);
	} else {
		flush_assert_cnt(self);
		buf = kmalloc(MAX_PRINTF, GFP_KERNEL);
		if (!buf) {
			terr("file %s line %d: Unable to allocate memory for the error report!",
			     file, line);
			goto out;
		}
		va_start(ap, fmt);
		len = vsnprintf(buf, MAX_PRINTF - 1, fmt, ap);
		buf[len] = 0;
		va_end(ap);
		if (self->skb) {
			nla_put_u32(self->skb, KTF_A_STAT, result);
			nla_put_string(self->skb, KTF_A_FILE, file);
			nla_put_u32(self->skb, KTF_A_NUM, line);
			nla_put_string(self->skb, KTF_A_STR, buf);
		}
		(void)snprintf(bufprefix, sizeof(bufprefix) - 1,
				"file %s line %d: result %d: ", file, line,
				result);
		tlog(T_PRINTK, "%s%s", bufprefix, buf);
		tlogs(T_STACKD, dump_stack());

		/* Multiple threads may try to update log */
		spin_lock_irqsave(&assert_lock, flags);
		(void)strncat(self->log, bufprefix, KTF_MAX_LOG);
		(void)strncat(self->log, buf, KTF_MAX_LOG);
		spin_unlock_irqrestore(&assert_lock, flags);
		kfree(buf);
	}
out:
	return result;
}
EXPORT_SYMBOL(_ktf_assert);

/* Add a test to a testcase:
 * Tests are represented by ktf_test objects that are linked into
 * a per-test case map TCase:tests map.
 */
void  _ktf_add_test(struct __test_desc td, struct ktf_handle *th,
		    int _signal, int allowed_exit_value,
		    int start, int end)
{
	struct ktf_case *tc = NULL;
	struct ktf_test *t;
	char *log;

	if (ktf_handle_version_check(th))
		return;

	log = kzalloc(KTF_MAX_LOG, GFP_KERNEL);
	if (!log)
		return;

	t = kzalloc(sizeof(*t), GFP_KERNEL);
	if (!t) {
		kfree(log);
		return;
	}
	t->tclass = td.tclass;
	t->name = td.name;
	t->fun = td.fun;
	t->start = start;
	t->end = end;
	t->handle = th;
	t->log = log;

	mutex_lock(&tc_lock);
	tc = ktf_case_find_create(td.tclass);
	if (!tc || ktf_map_elem_init(&t->kmap, td.name) ||
	    ktf_map_insert(&tc->tests, &t->kmap)) {
		terr("Failed to add test %s from %s to test case \"%s\"",
		     td.name, td.file, td.tclass);
		if (tc)
			ktf_case_put(tc);
		mutex_unlock(&tc_lock);
		kfree(log);
		kfree(t);
		return;
	}

	ktf_debugfs_create_test(t);

	tlog(T_LIST, "Added test \"%s.%s\" start = %d, end = %d",
	     td.tclass, td.name, start, end);

	/* Now since we no longer reference tc/t outside of global map of test
	 * cases and per-testcase map of tests, drop their refcounts.  This
	 * is safe to do as refcounts are > 0 due to references for map
	 * storage and debugfs.
	 */
	ktf_test_put(t);
	ktf_case_put(tc);
	mutex_unlock(&tc_lock);
}
EXPORT_SYMBOL(_ktf_add_test);

void ktf_run_hook(struct sk_buff *skb, struct ktf_context *ctx,
		  struct ktf_test *t, u32 value,
		void *oob_data, size_t oob_data_sz)
{
	int i;

	t->log[0] = '\0';
	t->skb = skb;
	t->data = oob_data;
	t->data_sz = oob_data_sz;
	for (i = t->start; i < t->end; i++) {
		if (!ctx && t->handle->require_context) {
			terr("Test %s.%s requires a context, but none configured!",
			     t->tclass, t->name);
			continue;
		}
		/* No need to bump refcnt, this is just for debugging.  Nothing
		 * should reference the testcase via the handle's current test
		 * pointer.
		 */
		t->handle->current_test = t;
		tlogs(T_DEBUG,
		      printk(KERN_INFO "Running test %s.%s", t->tclass, t->name);
			if (ctx)
				printk("_%s", ktf_context_name(ctx));
			printk("[%d:%d]\n", t->start, t->end);
		);
		ktime_get_ts64(&t->lastrun);
		t->fun(t, ctx, i, value);
		flush_assert_cnt(t);
	}
	t->handle->current_test = NULL;
}

/* Clean up all tests associated with a ktf_handle */

void ktf_test_cleanup(struct ktf_handle *th)
{
	struct ktf_test *t;
	struct ktf_case *tc;

	/* Clean up tests which are associated with this handle.
	 * It's possible multiple modules contribute tests to a test case,
	 * so we can't just do this on a per-testcase basis.
	 */
	mutex_lock(&tc_lock);

	tc = ktf_map_first_entry(&test_cases, struct ktf_case, kmap);
	while (tc) {
		/* FIXME - this is inefficient. */
		t = ktf_map_first_entry(&tc->tests, struct ktf_test, kmap);
		while (t) {
			if (t->handle == th) {
				tlog(T_DEBUG, "ktf: delete test %s.%s",
				     t->tclass, t->name);
				/* removes ref for debugfs */
				ktf_debugfs_destroy_test(t);
				/* removes ref for testset map of tests */
				ktf_map_remove_elem(&tc->tests, &t->kmap);
				/* now remove our reference which we get
				 * from ktf_map_[first|next]_entry().
				 * This final reference should result in
				 * the test being freed.
				 */
				ktf_test_put(t);
				/* Need to reset to root */
				t = ktf_map_first_entry(&tc->tests,
							struct ktf_test, kmap);
			} else {
				t = ktf_map_next_entry(t, kmap);
			}
		}
		/* If no modules have tests for this test case, we can
		 * free resources safely.
		 */
		if (ktf_case_test_count(tc) == 0) {
			ktf_debugfs_destroy_testset(tc);
			ktf_map_remove_elem(&test_cases, &tc->kmap);
			ktf_case_put(tc);
			tc = ktf_map_first_entry(&test_cases, struct ktf_case,
						 kmap);
		} else {
			tc = ktf_map_next_entry(tc, kmap);
		}
	}
	mutex_unlock(&tc_lock);
}
EXPORT_SYMBOL(ktf_test_cleanup);

int ktf_cleanup(void)
{
	struct ktf_test *t;
	struct ktf_case *tc;

	ktf_cov_cleanup();

	/* Unloading of dependencies means we should have no testcases/tests. */
	mutex_lock(&tc_lock);
	ktf_for_each_testcase(tc) {
		twarn("(memory leak) test set %s still active at unload!", ktf_case_name(tc));
		ktf_testcase_for_each_test(t, tc) {
			twarn("(memory leak) test set %s still active with test %s at unload!",
			      ktf_case_name(tc), t->name);
		}
		return -EBUSY;
	}
	ktf_debugfs_cleanup();
	mutex_unlock(&tc_lock);
	return 0;
}
