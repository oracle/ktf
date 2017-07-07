#ifndef NL_H
#define NL_H
#include <linux/list.h>
#include "ktf_map.h"
#include "kcheck.h"

int ktf_nl_register(void);
void ktf_nl_unregister(void);

struct ktf_case {
	struct ktf_map_elem kmap;  /* Linkage for ktf_map */
	struct list_head test_list; /* A list of tests to run */
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

/* The list of handles that have contexts associated with them */
extern struct list_head context_handles;

#endif
