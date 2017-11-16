#ifndef _TEST_H_
#define _TEST_H_

extern "C" {
#include "cutest-1.5/CuTest.h"
};


extern CuSuite *all_test_suites;


#define CU_TEST(func) \
	static void func(CuTest *); \
	static test_reg_t test_reg_##func(CuTestNew(#func, &func)); \
	static void func(CuTest *test)
	

struct test_reg_t {
	test_reg_t(CuTest *test) {
		TRACE("%s()\n", test->name);
		CuSuiteAdd(all_test_suites, test);
	}
};

#endif
