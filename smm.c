/* SPDX-License-Identifier: Apache-2.0 */
/* Simple Memory Memory (lib): A simple first fit memory algorithm */

#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include "smm.h"

#define SMM_HEAD_TAG 0xE5E5
#define SMMB_HEAD_FREE_TAG 0x5E5E
#define SMMB_HEAD_ALLOCED_TAG 0xAAAA

/* todo: free block need not to be the same as allocated block */
struct smmb_head {
	int tag;
	size_t size;
	struct smmb_head *next;
};

/* todo: make alignment */
struct smm_head {
	int tag;
	int align_mask;
	struct smmb_head *freelist;
};

static size_t __aligned_size(size_t sz, int align_mask)
{
	return (sz + align_mask) ^ align_mask;
}

/**
 * Initial a continue memory region to be managed by smm.
 *
 * @pt_addr the first address of the managed memory region
 * @size size of the region
 * @align_mask mask for address mask, 
 *             e.g. 0xFFF for aligning the memory block to 4K boundary
 */
int smm_init(void *pt_addr, size_t size, int align_mask)
{
	struct smm_head *h = pt_addr;
	struct smmb_head *bh;
	size_t hs = __aligned_size(sizeof(*h), align_mask);
	size_t bs = __aligned_size(sizeof(*bh), align_mask);

	if (size < hs + bs)
		return -ENOMEM;

	h->tag = SMM_HEAD_TAG;
	h->align_mask = align_mask;
	bh = h->freelist = pt_addr + hs;
	bh->tag = SMMB_HEAD_FREE_TAG;
	bh->next = NULL;
	bh->size = size - hs;
	
	return 0;
}

void *smm_alloc(void *pt_addr, size_t size)
{
	struct smm_head *h = pt_addr;
	struct smmb_head **bhp = &h->freelist;
	struct smmb_head *bh_new, *bh = h->freelist;
	size_t sz = __aligned_size(size + sizeof(*bh), h->align_mask);

	assert(h->tag == SMM_HEAD_TAG);

	while (bh) {
		assert(bh->tag == SMMB_HEAD_FREE_TAG);

		if (bh->size >= sz) {
			/* gotcha */
			if (bh->size < sz + sizeof(*bh)) {
				*bhp = bh->next;
				bh->tag = SMMB_HEAD_ALLOCED_TAG;
			} else {
				bh_new = (void *)bh + sz;
				bh_new->tag = SMMB_HEAD_FREE_TAG;
				bh_new->size = bh->size - sz;
				bh_new->next = bh->next;
				*bhp = bh_new;
				bh->size = sz;
			}

			bh->tag = SMMB_HEAD_ALLOCED_TAG;
			return bh + sizeof(*bh);
		}

		bhp = &(bh->next);
		bh = bh->next;
	}

	return NULL;
}

static inline _Bool __merge_free_block(struct smmb_head *h1,
				      struct smmb_head *h2)
{
	if ((void *)h1 + h1->size == h2) {
		h1->size += h2->size;
		h1->next = h2->next;
		return true;
	}

	return false;
}

void smm_free(void *pt_addr, void *ptr)
{
	struct smm_head *h = pt_addr;
	struct smmb_head **bhp = &h->freelist;
	struct smmb_head *bh = h->freelist;
	struct smmb_head *bh_cur = ptr - sizeof(struct smmb_head);

	assert(h->tag == SMM_HEAD_TAG);
	assert(bh_cur->tag == SMMB_HEAD_ALLOCED_TAG);

	while (bh) {
		assert(bh->tag == SMMB_HEAD_FREE_TAG);

		if ((void *)bh + bh->size == bh_cur) {
			bh->size += bh_cur->size;
			(void)__merge_free_block(bh, bh->next);
		} else if (bh_cur > bh) {
			*bhp = bh_cur;
			bh_cur->tag = SMMB_HEAD_FREE_TAG;

			if (!__merge_free_block(bh_cur, bh))
				bh_cur->next = bh;
		}

		bhp = &(bh->next);
		bh = bh->next;
	}

	assert(false);
}

void *smm_realloc(void *pt_addr, void *ptr, size_t size)
{
	/* there is no benefit to do reallocation in this algorithm */
	assert(false);
}
