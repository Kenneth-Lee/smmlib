#include <stdlib.h>
#include <stdio.h>
#include "../smm.h"
#include "ut.c"

#define PT_SIZE 1024*4
#define PTRN 10

void *ptrs[PTRN];
void *pt;

void testcase1(void) {
	int i;

	ut_assert_str(!smm_init(pt, PT_SIZE, 0xf), "smm_init");

	for (i=0; i<PTRN; i++) {
		ptrs[i] = smm_alloc(pt, 10);
	}

	ut_assert(smm_get_freeblock_num(pt)==1);

	smm_free(pt, ptrs[3]);
	ut_assert_str(((intptr_t)ptrs[3] & 0xf)==0, "aligment err %lx", ptrs[3]);
	ut_assert(smm_get_freeblock_num(pt)==2);

	smm_free(pt, ptrs[5]);
	ut_assert(((intptr_t)ptrs[5] & 0xf)==0);
	ut_assert(smm_get_freeblock_num(pt)==3);

	smm_free(pt, ptrs[4]);
	ut_assert(smm_get_freeblock_num(pt)==2);

	ptrs[3] = smm_alloc(pt, 10);
	ut_assert(smm_get_freeblock_num(pt)==2);

	ptrs[4] = smm_alloc(pt, 10);
	ut_assert(smm_get_freeblock_num(pt)==2);

	ptrs[5] = smm_alloc(pt, 10);
	ut_assert_str(smm_get_freeblock_num(pt)==1, "free block=%d", smm_get_freeblock_num(pt));

	for (i=0; i<PTRN; i++) {
		smm_free(pt, ptrs[i]);
	}
	ut_assert(smm_get_freeblock_num(pt)==1);
}

void testcase2(void) {
	ut_assert_str(!smm_init(pt, 1200, 0xf), "smm_init");

	ptrs[0] = smm_alloc(pt, 600);
	ut_assert(ptrs[0]);
	ut_assert(smm_get_freeblock_num(pt)==1);

	ptrs[1] = smm_alloc(pt, 600);
	ut_assert(!ptrs[1]);

	smm_free(pt, ptrs[0]);
	ut_assert(smm_get_freeblock_num(pt)==1);

	ptrs[1] = smm_alloc(pt, 700);
	ut_assert(ptrs[1]);
}

void testcase3(void) {
	ut_assert_str(!smm_init(pt, 120, 0xf), "smm_init");
	ptrs[0] = smm_alloc(pt, 0);
	ut_assert_str(((intptr_t)ptrs[0] & 0xf)==0, "alignment");
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
