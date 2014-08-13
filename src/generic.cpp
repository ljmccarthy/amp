/*
  This is a not-very-clever implementation of multi-methods
  which doesn't support inheritance.
*/

#include <stdint.h>
#include <string.h>
#include "lisp.h"

struct GenericTable {
	uint16_t count, power;
};

static int
bsearch_uint16(uint16_t key, const uint16_t *arr, size_t num)
{
	int lo = 0, hi = num-1;

	while (lo <= hi) {
		const int i = (lo + hi) / 2;
		const int d = key - arr[i];
		if (d < 0)
			hi = i-1;
		else if (d > 0)
			lo = i+1;
		else
			return i;
	}
	return ~lo;
}

/* count must be multiple of sizeof(void *) for pointer array alignment */

static inline uint16_t *
GT_TYPE_ARRAY(GenericTable *gt)
{
	return (uint16_t *) (gt + 1);
}

static inline void **
GT_PTR_ARRAY(GenericTable *gt)
{
	return (void **) (GT_TYPE_ARRAY(gt) + (1 << gt->power));
}

#define SIZEOF_GT(n) \
	(sizeof(GenericTable) + (n) * (sizeof(uint16_t) + sizeof(void *)))

static GenericTable *
gt_alloc(unsigned count, unsigned power)
{
	GenericTable *t = (GenericTable *) GC_object::operator new(SIZEOF_GT(1 << power));
	t->count = count;
	t->power = power;
	return t;
}

static int
gt_bsearch(GenericTable *t, uint16_t key)
{
	return bsearch_uint16(key, GT_TYPE_ARRAY(t), t->count);
}

static GenericTable *
gt_shuffle(GenericTable *t, unsigned idx)
{
	if (t->count < 1 << t->power) {
		memmove(GT_TYPE_ARRAY(t) + idx + 1,
		        GT_TYPE_ARRAY(t) + idx,
		        (t->count - idx) * sizeof(uint16_t));
		memmove(GT_PTR_ARRAY(t) + idx + 1,
		        GT_PTR_ARRAY(t) + idx,
		        (t->count - idx) * sizeof(void *));
		t->count++;
		return t;
	}
	else {
		GenericTable *n = gt_alloc(t->count+1, t->power+1);
		memcpy(GT_TYPE_ARRAY(n),
		       GT_TYPE_ARRAY(t),
		       idx * sizeof(uint16_t));
		memcpy(GT_TYPE_ARRAY(n) + idx + 1,
		       GT_TYPE_ARRAY(t) + idx,
		       (t->count - idx) * sizeof(uint16_t));
		memcpy(GT_PTR_ARRAY(n),
		       GT_PTR_ARRAY(t),
		       idx * sizeof(void *));
		memcpy(GT_PTR_ARRAY(n) + idx + 1,
		       GT_PTR_ARRAY(t) + idx,
		       (t->count - idx) * sizeof(void *));
		return n;
	}
}

static unsigned
gt_insert(GenericTable **tp, uint16_t key)
{
	GenericTable *t = *tp;
	if (!t) {
		t = *tp = gt_alloc(1, 2);
		GT_TYPE_ARRAY(t)[0] = key;
		GT_PTR_ARRAY(t)[0] = NULL;
		return 0;
	}
	int idx = gt_bsearch(t, key);
	if (idx < 0) {
		idx = ~idx;
		t = *tp = gt_shuffle(t, idx);
		GT_TYPE_ARRAY(t)[idx] = key;
		GT_PTR_ARRAY(t)[idx] = NULL;
	}
	return idx;
}

void
Generic::insert(uint16_t *types, void *ptr)
{
	GenericTable **tp = &table;

	for (unsigned arg = 0; arg < darity; arg++) {
		unsigned idx = gt_insert(tp, types[arg]);
		tp = (GenericTable **) GT_PTR_ARRAY(*tp) + idx;
	}
	*(void **)tp = ptr;
}

void *
Generic::lookup(uint16_t *types)
{
	GenericTable *t = table;

	for (unsigned arg = 0; arg < darity && t; arg++) {
		int idx = gt_bsearch(t, types[arg]);
		if (idx < 0)
			return default_value;
		t = (GenericTable *) GT_PTR_ARRAY(t)[idx];
	}
	return t;
}
