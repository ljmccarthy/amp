#include "lisp.h"
#include "ast.h"

struct Frame {
	Frame *up;
	Value slot[];
};

static Frame *
make_frame(unsigned nslots)
{
	return (Frame *) GC_object::operator new(
		sizeof(Frame) + nslots*sizeof(Value));
}

static inline Value
fetch(Frame *f, unsigned level, unsigned offset)
{
	while (level--)
		f = f->up;
	return f->slot[offset];
}

static Value
apply(Value fun, Frame *args, int nargs)
{
	int arity;

	if (!is_procedure(fun))
		goto err;

	arity = as_procedure(fun)->arity;
	if (arity >= 0 && nargs != arity)
		arity_error(as_procedure(fun));
	if (arity < 0 && nargs < ~arity)
		arity_error(as_procedure(fun));

	if (is_ast_procedure(fun)) {
		/* cons-up variable args */
		if (arity < 0) {
			Value varargs = NIL;
			while (nargs-- > ~arity)
				varargs = cons(args->slot[nargs], varargs);
			args->slot[~arity] = varargs;
		}
		args->up = as_ast_procedure(fun)->env;
		return execute(as_ast_procedure(fun)->body, args);
	}
	if (is_c_procedure(fun))
		return as_c_procedure(fun)->proc(nargs, args->slot);
	if (is_cc_procedure(fun))
		return as_cc_procedure(fun)->proc(
			as_ptr(fun), nargs, args->slot);
err:
	fun_type_error();
}

static inline unsigned
frame_size(Value fun, int nargs)
{
	/* need extra slot for nil varargs list when no varargs are given */
	//&& as_ast_procedure(fun)->arity < 0
	if (is_ast_procedure(fun) && nargs < -as_ast_procedure(fun)->arity)
		return nargs+1;
	return nargs;
}

Value
execute(Expr *exp, Frame *env)
{
	switch (exp->type) {
	case Expr::LIT:
		return ((Lit *)exp)->value;
	case Expr::LOCAL_REF: {
		LocalRef *lref = (LocalRef *) exp;
		return fetch(env, lref->level, lref->offset);
		}
	case Expr::MACRO_REF:
	case Expr::MODULE_REF: {
		ModuleRef *mref = (ModuleRef *) exp;
		if (mref->value == UNDEFINED)
			unbound_error(mref->name);
		return mref->value;
		}
	case Expr::COND: {
		Cond *cond = (Cond *) exp;
		return execute(execute(cond->pred, env) != _F
		               ? cond->then : cond->other, env);
		}
	case Expr::SEQ: {
		Seq *seq = (Seq *) exp;
		Value ret = NIL;
		for (unsigned i = 0; i < seq->count; i++)
			ret = execute(seq->expr[i], env);
		return ret;
		}
	case Expr::APP: {
		App *app = (App *) exp;
		unsigned nargs = app->args->count;
		Value fun = execute(app->fun, env);
		Frame *args = make_frame(frame_size(fun, nargs));
		for (unsigned i = 0; i < nargs; i++)
			args->slot[i] = execute(app->args->expr[i], env);
		return apply(fun, args, nargs);
		}
	case Expr::ABS: {
		Abs *abs = (Abs *) exp;
		return make_ast_procedure(sym_at_lambda, abs->arity,
		                          abs->body, env);
		}
	case Expr::DEFINE: {
		Define *def = (Define *) exp;
		Value value = execute(def->value, env);
		def->mod->define(def->name, value);
		if (is_procedure(value))
			as_procedure(value)->name = def->name;
		return value;
		}
	case Expr::DEFINE_MACRO: {
		DefineMacro *def = (DefineMacro *) exp;
		Procedure *proc = as_procedure(execute(def->body, env));
		def->mod->macro_define(def->name, proc);
		proc->name = def->name;
		return proc;
		}
	}
	return NIL;
}

Value
eval(Value exp, Module *mod)
{
	return execute(compile(exp, mod));
}

Value
apply_arglist(Value fun, Value arglist)
{
	unsigned nargs = length(arglist);
	Frame *args = make_frame(frame_size(fun, nargs));
	for (unsigned i = 0; i < nargs; i++, arglist = cdr(arglist))
		args->slot[i] = car(arglist);
	return apply(fun, args, nargs);
}
