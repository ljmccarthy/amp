/* general list operations */

#include "lisp.h"

unsigned
length(Value p)
{
	unsigned n = 0;
	for (; is_pair(p); p = cdr(p))
		n++;
	return n;
}

/* Reverse list in-place. */
Value
reverse(Value p, Value t)
{
	while (is_pair(p)) {
		Value r = cdr(p);
		cdr(p) = t;
		t = p;
		p = r;
	}
	return t;
}

bool
is_symbol_list(Value p)
{
	for (; is_pair(p); p = cdr(p))
		if (!is_symbol(car(p)))
			return false;
	if (!is_nil(p))
		return false;
	return true;
}

int
memq_index(Value key, Value alist)
{
	Value p = alist;
	for (unsigned i = 0; is_pair(p); p = cdr(p), i++)
		if (car(p) == key)
			return i;
	return -1;
}
