#include <cstdio>
#include "lisp.h"

#define prints(s) fputs(s, stdout)

static void
print_list(Value x)
{
	putchar('(');
	print(car(x));
	x = cdr(x);
	for (; is_pair(x); x = cdr(x)) {
		putchar(' ');
		print(car(x));
	}
	if (x != NIL) {
		prints(" . ");
		print(x);
	}
	putchar(')');
}

static void
print_string(String *x)
{
	putchar('"');
	prints(x->value);
	putchar('"');
}

void print(Value x)
{
	if (is_nil(x))
		prints("nil");
	else if (is_eof(x))
		printf("#eof");
	else if (is_fixnum(x))
		printf("%d", as_fixnum(x));
	else if (is_bool(x))
		printf("%s", as_bool(x) ? "true" : "false");
	else if (is_char(x))
		printf("'%c'", as_char(x));
	else if (is_pair(x))
		print_list(x);
	else if (is_symbol(x))
		prints(as_symbol(x)->value);
	else if (is_string(x))
		print_string(as_string(x));
	else if (is_procedure(x))
		printf("#<procedure %s>", as_procedure(x)->name->value);
	else if (is_module(x))
		printf("#<module>");
	else if (is_type(x))
		printf("#<type %s>", as_type(x)->name->value);
	else if (is_ptr(x))
		printf("#<object %p>", as_ptr(x));
	else if (is_undefined(x))
		printf("#undefined");
	else
		printf("#ufo");
}

void println(Value x)
{
	print(x);
	putchar('\n');
}
