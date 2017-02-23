#ifndef SIF_TEST_NL_H
#define SIF_TEST_NL_H
#include <linux/list.h>
#include "ktf_map.h"

int ktf_nl_register(void);
void ktf_nl_unregister(void);

struct ktf_case {
	struct ktf_map_elem kmap;  /* Linkage for ktf_map */
	struct list_head fun_list; /* A list of functions to run */
};

extern struct ktf_map test_cases;

static inline const char *tc_name(struct ktf_case *tc)
{
	return tc->kmap.name;
}

/* Current total number of test cases defined */
size_t ktf_case_count(void);


/* Called upon ktf unload to clean up test cases */
int ktf_cleanup(void);

struct fun_hook {
	const char* tclass; /* test class name */
	const char* name; /* Name of the test */
	TFun fun;
	int start; /* Start and end value to argument to fun */
	int end;   /* Defines number of iterations */
	struct ktf_handle *handle; /* Handler for owning module */
	struct list_head flist; /* linkage for all tests */
	struct list_head hlist; /* linkage for tests for a specific module */
};

/* The list of handles that have contexts associated with them */
extern struct list_head context_handles;

#endif
