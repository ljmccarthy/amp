/*
  atom.cpp - Atomic symbols
  Luke McCarthy (c) 2006
*/

#include <cstring>
#include "lisp.h"
#include "util.h"

static Dict symbols;
Symbol
	*sym_quote, *sym_quasiquote, *sym_unquote, *sym_unquote_splicing,
	*sym_lambda, *sym_if, *sym_begin, *sym_define, *sym_define_macro,
	*sym_at_lambda, *sym_at_constructor, *sym_at_accessor, *sym_at_mutator,
	*sym_this_module;

String::String(const char *str, size_t len, TypeCode typecode)
	: Object(typecode), len(len)
{
	if (is_static(str))
		this->value = str;
	else {
		char *p = (char *) GC_MALLOC_ATOMIC(len+1);
		memcpy(p, str, len);
		p[len] = 0;
		this->value = p;
	}
}

/*
  Make a symbol from a string.
  make_symbol(x) == make_symbol(y) <=> strcmp(x,y) == 0
*/
Symbol *
make_symbol(const char *str)
{
	Symbol *sym = (Symbol *) symbols.lookup(str);
	if (!sym) {
		sym = new Symbol(str, strlen(str));
		symbols.define(sym->value, sym);
	}
	return sym;
}

INIT {
	sym_quote = make_symbol("quote");
	sym_quasiquote = make_symbol("quasiquote");
	sym_unquote = make_symbol("unquote");
	sym_unquote_splicing = make_symbol("unquote-splicing");
	sym_lambda = make_symbol("lambda");
	sym_if = make_symbol("if");
	sym_begin = make_symbol("begin");
	sym_define = make_symbol("define");
	sym_define_macro = make_symbol("define-macro");
	sym_at_lambda = make_symbol("@lambda");
	sym_at_constructor = make_symbol("@constructor");
	sym_at_accessor = make_symbol("@accessor");
	sym_at_mutator = make_symbol("@mutator");
	sym_this_module = make_symbol("this-module");
}
