#ifndef SIF_TEST_NL_H
#define SIF_TEST_NL_H
#include <linux/list.h>


int ktest_nl_register(void);
void ktest_nl_unregister(void);

/* Current max number of (named) tests, increase when needed.. */
#define MAX_TEST_CASES 100
#define MAX_LEN_TEST_NAME 64


/* Number of elements in check_test_case[] */
extern int check_test_cnt;

struct TCase {
	char name[MAX_LEN_TEST_NAME+1];
	struct list_head fun_list; /* A list of functions to run */
};

struct fun_hook {
	const char* tclass; /* test class name */
	const char* name; /* Name of the test */
	TFun fun;
	int start; /* Start and end value to argument to fun */
	int end;   /* Defines number of iterations */
	struct list_head flist; /* linkage for all tests */
	struct list_head hlist; /* linkage for tests for a specific module */
};

/* The array of test cases defined */
extern TCase check_test_case[];

#endif
