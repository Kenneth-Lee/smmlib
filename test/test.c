#include <stdlib.h>
#include <stdio.h>
#include "../smm.h"

#define SYS_ERR_COND(cond, msg)		\
do {					\
	if (cond) {			\
		perror(msg);		\
		exit(EXIT_FAILURE);	\
	}				\
} while (0)

#define PT_SIZE (4096*10)

int main(void) {
	int ret;
	void *ptr;
	void *pt = malloc(PT_SIZE);
	SYS_ERR_COND(!pt, "malloc");

	ret = smm_init(pt, PT_SIZE, 0xf);
	SYS_ERR_COND(ret, "smm_init");

	ptr = smm_alloc(pt, 10);
	SYS_ERR_COND(!ptr, "smm_alloc");

	smm_free(pt, ptr);
	return 0;
}
