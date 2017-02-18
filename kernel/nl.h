#ifndef SIF_TEST_NL_H
#define SIF_TEST_NL_H
#include <linux/list.h>
#include "ktest_map.h"

int ktest_nl_register(void);
void ktest_nl_unregister(void);

struct ktest_case {
	struct ktest_map_elem kmap;  /* Linkage for ktest_map */
	struct list_head fun_list; /* A list of functions to run */
};

extern struct ktest_map test_cases;

static inline const char *tc_name(struct ktest_case *tc)
{
	return tc->kmap.name;
}

/* Current total number of test cases defined */
size_t ktest_case_count(void);


/* Called upon ktest unload to clean up test cases */
int ktest_cleanup(void);

struct fun_hook {
	const char* tclass; /* test class name */
	const char* name; /* Name of the test */
	TFun fun;
	int start; /* Start and end value to argument to fun */
	int end;   /* Defines number of iterations */
	struct ktest_handle *handle; /* Handler for owning module */
	struct list_head flist; /* linkage for all tests */
	struct list_head hlist; /* linkage for tests for a specific module */
};

/* The list of handles that have contexts associated with them */
extern struct list_head context_handles;

#endif
