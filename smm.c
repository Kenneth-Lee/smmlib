/* SPDX-License-Identifier: Apache-2.0 */
/* Simple Memory Memory (lib): A simple first fit memory algorithm */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

//#define DO_LOG_DEBUG
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
	/*
	DEBUG("__aligned_size: %lx, %x, %x => %lx\n",
	       sz, align_mask, ~align_mask, (sz + align_mask) & ~align_mask);
	*/
	return (sz + align_mask) & ~align_mask;
}

/**
 * Initial a continue memory region to be managed by smm.
 *
 * @pt_addr the first address of the managed memory region
 * @size size of the region
 * @align_mask mask for address mask, 
 *             e.g. 0xFFF for aligning the memory block to 4K boundary
 *             (but the control block itself will use a 4K page too)
 */
int smm_init(void *pt_addr, size_t size, int align_mask)
{
	struct smm_head *h = pt_addr;
	struct smmb_head *bh;
	size_t hs = __aligned_size(sizeof(*h), align_mask);
	size_t bs = __aligned_size(sizeof(*bh), align_mask);

	if (((intptr_t)pt_addr & align_mask) != 0)
		return -EINVAL;

	if (size < hs + bs)
		return -ENOMEM;

	h->tag = SMM_HEAD_TAG;
	h->align_mask = align_mask;
	bh = h->freelist = pt_addr + hs;
	bh->tag = SMMB_HEAD_FREE_TAG;
	bh->next = NULL;
	bh->size = size - hs;
	DEBUG("init: %ld byte, head 0x%lx bytes, block head 0x%lx bytes, remain 0x%lx bytes\n",
	      size, hs, bs, bh->size);
	
	return 0;
}

void *smm_alloc(void *pt_addr, size_t size)
{
	struct smm_head *h = pt_addr;
	struct smmb_head **bhp = &h->freelist;
	struct smmb_head *bh_new, *bh;
	size_t bs = __aligned_size(sizeof(*bh), h->align_mask);
	size_t sz = __aligned_size(size, h->align_mask) + bs;
	//DEBUG("alloc 0x%lx for 0x%lx\n", sz, size);

	assert(h->tag == SMM_HEAD_TAG);

	bh = h->freelist;
	while (bh) {
		assert(bh->tag == SMMB_HEAD_FREE_TAG);

		//DEBUG("alloc try block(%p, %ld) for %ld\n", bh, bh->size, sz);
		if (bh->size >= sz) {
			/* gotcha */

			if (bh->size < sz+bs) {
				/* remaid size should at lease biger than block head */
				DEBUG("alloc: use the whole block %lx\n", (void *)bh-pt_addr);
				*bhp = bh->next;
			} else {
				bh_new = (void *)bh + sz;
				bh_new->tag = SMMB_HEAD_FREE_TAG;
				bh_new->size = bh->size - sz;
				bh_new->next = bh->next;
				*bhp = bh_new;
				bh->size = sz;
				DEBUG("alloc: split block %lx, new block %lx(%ld)\n",
				       (void *)bh-pt_addr, (void *)bh_new-pt_addr, bh_new->size);
			}

			bh->tag = SMMB_HEAD_ALLOCED_TAG;
			return (void *)bh + bs;
		}

		bhp = &(bh->next);
		bh = bh->next;
	}

	return NULL;
}

static inline _Bool __merge_free_block(void *pt_addr, struct smmb_head *h1,
				      struct smmb_head *h2)
{
	if ((void *)h1 + h1->size == h2) {
		DEBUG("merge: %lx, %lx\n", (void *)h1-pt_addr, (void *)h2-pt_addr);
		h1->size += h2->size;
		h1->next = h2->next;
		return true;
	}

	DEBUG("no merge: %lx, %lx\n", (void *)h1-pt_addr, (void *)h2-pt_addr);
	return false;
}

void smm_free(void *pt_addr, void *ptr)
{
	struct smm_head *h = pt_addr;
	struct smmb_head **bhp, *bh;
	size_t bs = __aligned_size(sizeof(*bh), h->align_mask);
	struct smmb_head *bh_cur = ptr - bs;

	assert(h->tag == SMM_HEAD_TAG);
	assert(bh_cur->tag == SMMB_HEAD_ALLOCED_TAG);

	if (!h->freelist) {
		h->freelist = bh_cur;
		bh_cur->tag = SMMB_HEAD_FREE_TAG;
		bh_cur->next = NULL;
		return;
	}

	bh = h->freelist;
	bhp = &h->freelist;
	while (bh) {
		assert(bh->tag == SMMB_HEAD_FREE_TAG);

		DEBUG("free: iterate bh=%lx(s=%ld), bh_cur=%lx\n", (void *)bh-pt_addr, bh->size,
		      (void *)bh_cur-pt_addr);

		if (bh_cur < bh) {
			*bhp = bh_cur;
			bh_cur->tag = SMMB_HEAD_FREE_TAG;

			if (!__merge_free_block(pt_addr, bh_cur, bh))
				bh_cur->next = bh;

			return;
		} else if ((void *)bh + bh->size == bh_cur) {
			bh->size += bh_cur->size;
			(void)__merge_free_block(pt_addr, bh, bh->next);
			return;
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

#ifndef NDEBUG
void smm_dump(void *pt_addr) {
	struct smm_head *h = pt_addr;
	struct smmb_head *bh = h->freelist;

	assert(h->tag == SMM_HEAD_TAG);

	DEBUG("dump pt %p: align_mask = 0x%x\n", h, h->align_mask);
	while (bh) {
		assert(bh->tag == SMMB_HEAD_FREE_TAG);
		DEBUG("freeblock(%p): sz=%ld\n", bh, bh->size);
		bh = bh->next;
	}
}

int smm_get_freeblock_num(void *pt_addr) {
	struct smm_head *h = pt_addr;
	struct smmb_head *bh = h->freelist;
	int ret = 0;

	assert(h->tag == SMM_HEAD_TAG);

	DEBUG("current free-blocks: ");
	while (bh) {
		DEBUG("0x%lx ", (void *)bh - pt_addr);
		assert(bh->tag == SMMB_HEAD_FREE_TAG);
		bh = bh->next;
		ret++;
	}
	DEBUG("\n");

	return ret;
}
#endif
