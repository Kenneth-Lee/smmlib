#include <stdlib.h>
#include <stdio.h>
#include "../smm.h"
#include "ut.c"

#define PT_SIZE 1024
#define PTRN 10

void *ptrs[PTRN];
void *pt;

void testcase1(void) {
	int i;

	ut_assert_str(!smm_init(pt, PT_SIZE, 0xf), "smm_init");

	for (i=0; i<PTRN; i++) {
		ptrs[i] = smm_alloc(pt, 10);
	}

	smm_free(pt, ptrs[3]);
	ut_assert(smm_get_freeblock_num(pt)==2);

	smm_free(pt, ptrs[5]);
	ut_assert(smm_get_freeblock_num(pt)==3);

	smm_free(pt, ptrs[4]);
	ut_assert(smm_get_freeblock_num(pt)==2);

	ptrs[3] = smm_alloc(pt, 10);
	ut_assert(smm_get_freeblock_num(pt)==2);

	ptrs[4] = smm_alloc(pt, 10);
	ut_assert(smm_get_freeblock_num(pt)==2);

	ptrs[5] = smm_alloc(pt, 10);
	ut_assert(smm_get_freeblock_num(pt)==1);

	for (i=0; i<PTRN; i++) {
		smm_free(pt, ptrs[i]);
	}
	ut_assert(smm_get_freeblock_num(pt)==1);
}

void testcase2(void) {
	ut_assert_str(!smm_init(pt, 120, 0xf), "smm_init");

	ptrs[0] = smm_alloc(pt, 60);
	ut_assert(ptrs[0]);

	ptrs[1] = smm_alloc(pt, 60);
	ut_assert(!ptrs[1]);

	smm_free(pt, ptrs[0]);

	ptrs[1] = smm_alloc(pt, 70);
	ut_assert(ptrs[1]);
}

void testcase3(void) {
	ut_assert_str(!smm_init(pt, 120, 0xf), "smm_init");
	ptrs[0] = smm_alloc(pt, 0);
	smm_free(pt, ptrs[0]);
	ut_assert(smm_get_freeblock_num(pt)==1);
}

int main(void) {
	pt = malloc(PT_SIZE);
	ut_assert_str(pt, "malloc");

	testcase1();
	testcase2();
	testcase3();

	printf("Test Success\n");
	return 0;
}
