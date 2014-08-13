/*
  dict.cpp - Ternary search tree dictionary
  Luke McCarthy (c) 2006, 2007
*/

#include "lisp.h"

struct tstnode_t : public GC_object {
	int ch;
	tstnode_t *lo, *eq, *hi;
	tstnode_t(int ch) : ch(ch), lo(0), eq(0), hi(0) {}
};

static tstnode_t *
tst_insert(tstnode_t *p, const char *s, const void *v)
{
	if (!p)
		p = new tstnode_t(*s);
	if (*s < p->ch)
		p->lo = tst_insert(p->lo, s, v);
	else if (*s > p->ch)
		p->hi = tst_insert(p->hi, s, v);
	else
		p->eq = *s ? tst_insert(p->eq, s+1, v) : (tstnode_t *) v;
	return p;
}

static void *
tst_lookup(tstnode_t *p, const char *s)
{
	while (p) {
		if (*s < p->ch)
			p = p->lo;
		else if (*s > p->ch)
			p = p->hi;
		else {
			if (*s++ == 0)
				return (void *) p->eq;
			p = p->eq;
		}
	}
	return NULL;
}

void
Dict::define(const char *key, const void *value)
{
//	printf("define(%s)\n", s);
	root = tst_insert(root, key, value);
}

void *
Dict::lookup(const char *key)
{
	return tst_lookup(root, key);
}
