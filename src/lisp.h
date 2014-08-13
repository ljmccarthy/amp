#ifndef LISP_H
#define LISP_H

#define UNUSED __attribute__((unused))
#define NORETURN __attribute__((noreturn))
#define INIT static __attribute__((constructor)) void __i_n_i_t__(void)

#include "value.h"
#include "objects.h"
#include <cstdio>

class Exit {
public:
	int code;
	inline Exit(int code = 0) : code(code) {}
};

class BaseError {};
class Error : public BaseError {};
class FatalError : public BaseError {};
class TypeError : public Error {};
class ParseError : public Error {};
class SyntaxError : public Error {};
class UnboundError : public Error {};
class ArityError : public Error {};

#define error(exn, msg) do {      \
	fputs(msg "\n", stderr);  \
	throw exn;                \
} while (0)

#define errorf(exn, fmt, ...) do {               \
	fprintf(stderr, fmt "\n", __VA_ARGS__);  \
	throw exn;                               \
} while (0)

#define syntax_error(s) \
	error(SyntaxError(), "syntax error: " s)

#define type_error(src) \
	errorf(TypeError(), "type error: %s", src)

#define fun_type_error() \
	error(TypeError(), "type error: attempt to apply non-procedure")

#define arity_error(proc) \
	errorf(ArityError(), "arity error: %s", (proc)->name->value)

#define unbound_error(sym) \
	errorf(UnboundError(), "unbound variable: %s", (sym)->value)

extern Symbol
	*sym_quote, *sym_quasiquote, *sym_unquote, *sym_unquote_splicing,
	*sym_lambda, *sym_if, *sym_begin, *sym_define, *sym_define_macro,
	*sym_at_lambda, *sym_at_constructor, *sym_at_accessor, *sym_at_mutator,
	*sym_this_module;

extern void
print(Value x);

extern void
println(Value x);

extern unsigned
length(Value p);

extern Value
reverse(Value p, Value t = NIL);

extern Symbol *
make_symbol(const char *value);

extern bool
is_symbol_list(Value p);

extern int
memq_index(Value key, Value alist);

extern Expr *
compile(Value exp, Module *mod);

extern Value
execute(Expr *exp, Frame *env=0);

extern void
init_types(void);

extern void
init_primitives(void);

extern void
primitives(Module *mod);

extern Value
eval(Value exp, Module *mod);

extern Value
apply_arglist(Value fun, Value arglist);

extern TypeCode
alloc_type_code(void);

#endif /* LISP_H */
