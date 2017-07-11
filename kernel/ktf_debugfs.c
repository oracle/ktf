#include <asm/unistd.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/debugfs.h>
#include "ktf.h"
#include "kcheck.h"

/* Create a debugfs representation of test sets/tests.  Hierarchy looks like
 * this:
 *
 * Path					Semantics
 * /sys/kernel/debug/ktf/run/<testset>		Run all tests in testset
 * /sys/kernel/debug/ktf/run/<testset>/<test>	Run specific test in testset
 * /sys/kernel/debug/ktf/results/<testset>	Show results of last run for
 *						testset
 *
 * /sys/kernel/debug/ktf/results/<testset>/<test>
 *						Show results of last run for
 *						test
 *
 */

struct dentry *ktf_debugfs_rootdir = NULL;
struct dentry *ktf_debugfs_rundir = NULL;
struct dentry *ktf_debugfs_resultsdir = NULL;

static void ktf_debugfs_print_result(struct seq_file *seq, struct ktf_test *t)
{
	struct timespec now;

	if (t && strlen(t->log) > 0) {
		getnstimeofday(&now);
		seq_printf(seq, "[%s/%s, %ld seconds ago] %s\n",
			   t->tclass, t->name,
			   now.tv_sec - t->lastrun.tv_sec, t->log);
	}
}

/* /sys/kernel/debug/ktf/results/<testset>-tests/<test> shows specific result */
static int ktf_debugfs_result(struct seq_file *seq, void *v)
{
	struct ktf_test *t = (struct ktf_test *)seq->private;

	ktf_debugfs_print_result(seq, t);

	return (0);
}

/* /sys/kernel/debug/ktf/results/<testset> shows all results for testset. */
static int ktf_debugfs_results_all(struct seq_file *seq, void *v)
{
	struct ktf_case *testset = (struct ktf_case *)seq->private;
	struct ktf_test *t;

	if (!testset)
		return (0);

	seq_printf(seq, "%s results:\n", ktf_case_name(testset));
	list_for_each_entry(t, &testset->test_list, tlist)
		ktf_debugfs_print_result(seq, t);

	return (0);
}

/* /sys/kernel/debug/ktf/run/<testset>-tests/<test> runs specific test. */
static int ktf_debugfs_run(struct seq_file *seq, void *v)
{
	struct ktf_test *t = (struct ktf_test *)seq->private;

	if (!t)
		return (0);

	ktf_run_hook(NULL, NULL, t, 0);
	ktf_debugfs_print_result(seq, t);

	return (0);
}

/* /sys/kernel/debug/ktf/run/<testset> runs all tests in testset. */
static int ktf_debugfs_run_all(struct seq_file *seq, void *v)
{
	struct ktf_case *testset = (struct ktf_case *)seq->private;
	struct ktf_test *t;

	if (!testset)
		return (0);

	seq_printf(seq, "Running %s\n", ktf_case_name(testset));
	list_for_each_entry(t, &testset->test_list, tlist) {
		ktf_run_hook(NULL, NULL, t, 0);
		ktf_debugfs_print_result(seq, t);
	}

	return (0);
}

static int ktf_run_test_open(struct inode *inode, struct file *file)
{
	struct ktf_test *t;

	if (!try_module_get(THIS_MODULE))
		return -EIO;

	t = (struct ktf_test *)inode->i_private;

	return single_open(file, ktf_debugfs_run, t);
}

static int ktf_debugfs_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return single_release(inode, file);
}

static struct file_operations ktf_run_test_fops = {
	.open = ktf_run_test_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = ktf_debugfs_release,
};

static int ktf_results_test_open(struct inode *inode, struct file *file)
{
	struct ktf_test *t;

	if (!try_module_get(THIS_MODULE))
		return -EIO;

	t = (struct ktf_test *)inode->i_private;

	return single_open(file, ktf_debugfs_result, t);
}

static struct file_operations ktf_results_test_fops = {
	.open = ktf_results_test_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = ktf_debugfs_release,
};

void ktf_debugfs_destroy_test(struct ktf_test *t)
{
	if (!t)
		return;

	if (t->debugfs.debugfs_results_test)
		debugfs_remove(t->debugfs.debugfs_results_test);
	if (t->debugfs.debugfs_run_test)
		debugfs_remove(t->debugfs.debugfs_run_test);
	memset(&t->debugfs, 0, sizeof (t->debugfs));
}

int ktf_debugfs_create_test(struct ktf_test *t)
{
	struct ktf_case *testset = ktf_case_find(t->tclass);

	if (!testset)
		return 0;

	memset(&t->debugfs, 0, sizeof (t->debugfs));

	t->debugfs.debugfs_results_test =
		debugfs_create_file(t->name, S_IFREG | 0444,
				 testset->debugfs.debugfs_results_test,
				 t, &ktf_results_test_fops);

	if (!t->debugfs.debugfs_results_test)
		return 0;

	t->debugfs.debugfs_run_test =
		debugfs_create_file(t->name, S_IFREG | 0444,
				 testset->debugfs.debugfs_run_test,
				 t, &ktf_run_test_fops);
	if (!t->debugfs.debugfs_run_test)
		ktf_debugfs_destroy_test(t);
	return (0);
}

static int ktf_results_testset_open(struct inode *inode, struct file *file)
{
	struct ktf_case *testset;

	if (!try_module_get(THIS_MODULE))
		return -EIO;

	testset = (struct ktf_case *)inode->i_private;

	return single_open(file, ktf_debugfs_results_all, testset);
}

static struct file_operations ktf_results_testset_fops = {
        .open = ktf_results_testset_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = ktf_debugfs_release,
};

static int ktf_run_testset_open(struct inode *inode, struct file *file)
{
	struct ktf_case *testset;

	if (!try_module_get(THIS_MODULE))
		return -EIO;

	testset = (struct ktf_case *)inode->i_private;

	return single_open(file, ktf_debugfs_run_all, testset);
}

static struct file_operations ktf_run_testset_fops = {
	.open = ktf_run_testset_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = ktf_debugfs_release,
};

int ktf_debugfs_create_testset(struct ktf_case *testset)
{
	char tests_subdir[KTF_DEBUGFS_NAMESZ];
	const char *name = ktf_case_name(testset);

	memset(&testset->debugfs, 0, sizeof (testset->debugfs));

	/* First add /sys/kernel/debug/ktf/[results|run]/<testset> */
	testset->debugfs.debugfs_results_testset =
		debugfs_create_file(name, S_IFREG | 0444,
			         ktf_debugfs_resultsdir,
				 testset, &ktf_results_testset_fops);
	if (!testset->debugfs.debugfs_results_testset)
		goto err;

	testset->debugfs.debugfs_run_testset =
		debugfs_create_file(name, S_IFREG | 0444,
				    ktf_debugfs_rundir,
				    testset, &ktf_run_testset_fops);
	if (!testset->debugfs.debugfs_run_testset)
		goto err;

	/* Now add parent directories for individual test result/run tests
	 * which live in
	 * /sys/kernel/debug/ktf/[results|run]/<testset>-tests/<testname>
	 */
	(void) snprintf(tests_subdir, sizeof (tests_subdir), "%s%s",
			name, KTF_DEBUGFS_TESTS_SUFFIX);

	testset->debugfs.debugfs_results_test =
		debugfs_create_dir(tests_subdir, ktf_debugfs_resultsdir);
	if (!testset->debugfs.debugfs_results_test)
		goto err;

	testset->debugfs.debugfs_run_test =
		debugfs_create_dir(tests_subdir, ktf_debugfs_rundir);
	if (!testset->debugfs.debugfs_run_test)
		goto err;

	return (0);
err:
	ktf_debugfs_destroy_testset(testset);

	return -ENOMEM;
}

void ktf_debugfs_destroy_testset(struct ktf_case *testset)
{
	printk(KERN_INFO "Destroying testset %s", ktf_case_name(testset));

	if (testset->debugfs.debugfs_run_testset)
		debugfs_remove(testset->debugfs.debugfs_run_testset);
	if (testset->debugfs.debugfs_run_test)
		debugfs_remove(testset->debugfs.debugfs_run_test);
	if (testset->debugfs.debugfs_results_testset)
		debugfs_remove(testset->debugfs.debugfs_results_testset);
	if (testset->debugfs.debugfs_results_test)
		debugfs_remove(testset->debugfs.debugfs_results_test);

}

void ktf_debugfs_cleanup(void)
{
	printk(KERN_INFO "Removing ktf debugfs dirs...");
	if (ktf_debugfs_rundir)
		debugfs_remove(ktf_debugfs_rundir);
	if (ktf_debugfs_resultsdir)
		debugfs_remove(ktf_debugfs_resultsdir);
	if (ktf_debugfs_rootdir)
		debugfs_remove(ktf_debugfs_rootdir);
}

void ktf_debugfs_init(void)
{
	ktf_debugfs_rootdir = debugfs_create_dir(KTF_DEBUGFS_ROOT, NULL);
	if (!ktf_debugfs_rootdir)
		goto err;
	ktf_debugfs_rundir = debugfs_create_dir(KTF_DEBUGFS_RUN,
						ktf_debugfs_rootdir);
	if (!ktf_debugfs_rundir)
		goto err;
	ktf_debugfs_resultsdir = debugfs_create_dir(KTF_DEBUGFS_RESULTS,
						    ktf_debugfs_rootdir);
	if (ktf_debugfs_resultsdir)
		return;
err:
	printk(KERN_INFO "Could not init %s\n", KTF_DEBUGFS_ROOT);
	ktf_debugfs_cleanup();
}
