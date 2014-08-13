#include <cstdlib>
#include <cstring>
#include "lisp.h"
#include "util.h"

/* static primitive information */
struct prim_info {
	const char *name;
	int arity;
	Value (*proc)(unsigned, Value *);
};

#define DEF_PRIM(name, symname, arity)              \
static Value name(unsigned, Value *);               \
static prim_info _##name = {symname, arity, name};  \
static Value \
name(UNUSED const unsigned nargs, UNUSED Value *const arg)

DEF_PRIM(prim_exit, "exit", 1)
{
	if (!is_fixnum(arg[0]))
		type_error("exit");
	throw Exit(as_fixnum(arg[0]));
}

DEF_PRIM(prim_make_record_type, "make-record-type", 2)
{
	if (!is_symbol(arg[0]) || !is_symbol_list(arg[1]))
		type_error("make-record-type");
	return new RecordType(alloc_type_code(), as_symbol(arg[0]), arg[1]);
}

DEF_PRIM(prim_constructor, "constructor", 1)
{
	if (!is_record_type(arg[0]))
		type_error("constructor");
	return as_record_type(arg[0])->constructor();
}

DEF_PRIM(prim_accessor, "accessor", 2)
{
	if (is_record_type(arg[0]) && is_symbol(arg[1]))
		return as_record_type(arg[0])->accessor(as_symbol(arg[1]));
	type_error("accessor");
}

DEF_PRIM(prim_mutator, "mutator", 2)
{
	if (!is_record_type(arg[0]) || !is_symbol(arg[1]))
		type_error("mutator");
	return as_record_type(arg[0])->mutator(as_symbol(arg[1]));
}

DEF_PRIM(prim_make_symbol, "make-symbol", ~1)
{
	size_t len = 0;

	for (unsigned i = 0; i < nargs; i++) {
		if (!is_symbol(arg[i]) && !is_string(arg[i]))
			type_error("make-symbol");
		len += as_string(arg[i])->len;
	}

	char buf[len+1];
	size_t pos = 0;

	for (unsigned i = 0; i < nargs; i++) {
		String *str = as_string(arg[i]);
		memcpy(buf+pos, str->value, str->len);
		pos += str->len;
	}
	buf[len] = 0;
	return make_symbol(buf);
}

DEF_PRIM(prim_eqv, "eqv?", 2)
{
	return make_bool(arg[0] == arg[1]);
}

DEF_PRIM(prim_not, "not", 1)
{
	return make_bool(arg[0] == _F);
}

int
compare(Value x, Value y)
{
	if (is_fixnum(x)) {
		if (!is_fixnum(y))
			goto err;
		return as_fixnum(x) - as_fixnum(y);
	}
	else if (is_string(x)) {
		if (!is_string(y))
			goto err;
		return strcmp(as_string(x)->value, as_string(y)->value);
	}
err:
	type_error("compare");
}

DEF_PRIM(prim_lt, "<", 2)
{
	return make_bool(compare(arg[0], arg[1]) < 0);
}

DEF_PRIM(prim_gt, ">", 2)
{
	return make_bool(compare(arg[0], arg[1]) > 0);
}

#define DEF_FIXNUM_OP(name, symname, op)        \
DEF_PRIM(name, symname, ~2)                     \
{                                               \
	if (!is_fixnum(arg[0]))                 \
		type_error(#op);                \
	Fixnum n = as_fixnum(arg[0]);           \
	for (unsigned i = 1; i < nargs; i++) {  \
		if (!is_fixnum(arg[i]))         \
			type_error(#op);        \
		n = n op as_fixnum(arg[i]);     \
	}                                       \
	return make_fixnum(n);                  \
}

DEF_FIXNUM_OP(prim_add, "+", +)
DEF_FIXNUM_OP(prim_sub, "-", -)  // TODO: accept 1 argument
DEF_FIXNUM_OP(prim_mul, "*", *)
DEF_FIXNUM_OP(prim_div, "/", /)

DEF_PRIM(prim_negate, "negate", 1)
{
	if (is_fixnum(arg[0]))
		return make_fixnum(-as_fixnum(arg[0]));
	type_error("negate");
}

DEF_PRIM(prim_length, "length", 1)
{
	return make_fixnum(length(arg[0]));
}

DEF_PRIM(prim_apply, "apply", 2)
{
	return apply_arglist(arg[0], arg[1]);
}

DEF_PRIM(prim_println, "println", 1)
{
	println(arg[0]);
	return NIL;
}

#define PUTS1(x) do {                                \
	if (is_string(x))                            \
		fputs(as_string(x)->value, stdout);  \
	else                                         \
		print(x);                            \
} while (0)

DEF_PRIM(prim_puts, "puts", ~0)
{
	if (nargs > 0) {
		PUTS1(arg[0]);
		for (unsigned i = 1; i < nargs; i++) {
			putchar(' ');
			PUTS1(arg[i]);
		}
	}
	putchar('\n');
	return NIL;
}

DEF_PRIM(prim_error, "error", ~0)
{
	prim_puts(nargs, arg);
	throw Error();
}

static int
compare_callback(const void *x, const void *y)
{
	return compare(car(Value(*(Object **) x)),
	               car(Value(*(Object **) y)));
}

DEF_PRIM(prim_sort_bang, "sort!", 1)
{
	if (is_nil(arg[0]))
		return NIL;

	/* copy pointers to consecutive pairs into array */
	unsigned n = length(arg[0]);
	malloc_ptr<Value> arr = (Value *) calloc(n, sizeof(Value));
	Value p = arg[0];
	for (unsigned i = 0; is_pair(p); p = cdr(p), i++)
		arr[i] = p;
	if (!is_nil(p))
		type_error("sort!");

	/* let stdlib do the dirty work */
	qsort(arr, n, sizeof(arr[0]), compare_callback);

	/* fix up cdrs */
	for (unsigned i = 0; i < n-1; i++)
		cdr(arr[i]) = arr[i+1];
	cdr(arr[n-1]) = NIL;
	return arr[0];
}

#define DEF_TYPE_PRED(name, symname, pred)  \
DEF_PRIM(name, symname, 1)                  \
{                                           \
	return make_bool(pred(arg[0]));     \
}

DEF_TYPE_PRED(prim_is_pair, "pair?", is_pair)
DEF_TYPE_PRED(prim_is_symbol, "symbol?", is_symbol)

static const struct prim_info
prim_table[] = {
	_prim_exit,
	_prim_make_record_type,
	_prim_constructor,
	_prim_accessor,
	_prim_mutator,
	_prim_make_symbol,
	_prim_eqv,
	_prim_not,
	_prim_lt,
	_prim_gt,
	_prim_add,
	_prim_sub,
	_prim_mul,
	_prim_div,
	_prim_negate,
	_prim_length,
	_prim_apply,
	_prim_println,
	_prim_puts,
	_prim_error,
	_prim_sort_bang,
	_prim_is_pair,
	_prim_is_symbol,
};

static Procedure *prim_objs[NELEMS(prim_table)];

void
init_primitives(void)
{
	for (unsigned i = 0; i < NELEMS(prim_table); i++)
		prim_objs[i] = new CProcedure(
			make_symbol(prim_table[i].name),
			prim_table[i].arity,
			prim_table[i].proc);
}

void
primitives(Module *mod)
{
	for (unsigned i = 0; i < NELEMS(prim_objs); i++)
		mod->define(prim_objs[i]->name, prim_objs[i]);

	mod->define(make_symbol("<pair>"), type_table[TC_PAIR]);
}
