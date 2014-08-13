#include "lisp.h"
#include "ast.h"

class Cenv {
	Cenv *const up;
	Module *const mod;
	Value const vars;
	int nvars;
public:
	explicit Cenv(Module *mod) : up(0), mod(mod), vars(NIL), nvars(0) {}
	Cenv(Cenv *up, Value vars);
	Expr *lookup(Symbol *name) const;
	Module *module() const { return mod; }
	bool toplevel() const { return !up; }
	int arity() const { return nvars; }
};

Cenv::Cenv(Cenv *up, Value vars)
	: up(up), mod(up->mod), vars(vars), nvars(0)
{
	Value p = vars;
	for (; is_pair(p); p = cdr(p), nvars++)
		if (!is_symbol(car(p)))
			goto err;
	if (!is_nil(p)) {
		if (!is_symbol(p))
			goto err;
		nvars = ~nvars;
	}
	return;
err:
	syntax_error("variable name must be a symbol");
}

Expr *
Cenv::lookup(Symbol *name) const
{
	unsigned level = 0;
	for (const Cenv *p = this; p; p = p->up, level++) {
		unsigned offset = 0;
		Value vp = p->vars;
		for (; is_pair(vp); vp = cdr(vp), offset++)
			if (car(vp) == name)
				return new LocalRef(level, offset);
		if (vp == name)
			return new LocalRef(level, offset);
	}
	return mod->lookup(name);
}

static Expr *
eval(Value exp, Cenv *env);

static Lit *
eval_lit(Value val)
{
	return new Lit(val);
}

static Lit *
eval_quote(Value exp)
{
	if (!is_pair(exp))
		syntax_error("quote");
	if (!is_nil(cdr(exp)))
		syntax_error("quote");
	return new Lit(car(exp));
}

static Cond *
eval_if(Value exp, Cenv *env)
{
	if (!is_pair(exp))
		syntax_error("if");
	Value pred = car(exp);
	if (!is_pair(exp = cdr(exp)))
		syntax_error("if");
	Value then = car(exp);
	if (!is_pair(exp = cdr(exp)))
		syntax_error("if");
	Value other = car(exp);
	if (!is_nil(cdr(exp)))
		syntax_error("if");

	return new Cond(eval(pred, env), eval(then, env), eval(other, env));
}

static Seq *
eval_seq(Value exp, Cenv *env)
{
	unsigned count = length(exp);
	Seq *seq = (Seq *) Seq::operator new(
		sizeof(Seq) + count * sizeof(Expr *));
	seq->type = Expr::SEQ;
	seq->count = count;
	for (unsigned i = 0; i < count; i++, exp = cdr(exp))
		seq->expr[i] = eval(car(exp), env);
	if (!is_nil(exp))
		syntax_error("terminated by non-nil");
	return seq;
}

static Abs *
eval_lambda(Value exp, Cenv *env)
{
	if (!is_pair(exp))
		syntax_error("lambda");
	Value formals = car(exp);
	if (!is_pair(exp = cdr(exp)))
		syntax_error("lambda: no body");
	Value body = exp;

	Cenv subenv(env, formals);
	return new Abs(subenv.arity(), eval_seq(body, &subenv));
}

static Define *
eval_define(Value exp, Cenv *env)
{
	if (!env->toplevel())
		syntax_error("define must be non-local");
	if (!is_pair(exp))
		syntax_error("define: missing name and value");
	Value name = car(exp);
	if (!is_pair(exp = cdr(exp)))
		syntax_error("define: missing value");

	Expr *value;
	if (!is_pair(name)) {
		if (!is_nil(cdr(exp)))
			syntax_error("define");
		value = eval(car(exp), env);
	}
	else {
		Cenv subenv(env, cdr(name));
		name = car(name);
		value = new Abs(subenv.arity(), eval_seq(exp, &subenv));
	}
	if (!is_symbol(name))
		syntax_error("define: name must be a symbol");

	return new Define(env->module(), as_symbol(name), value);
}

static DefineMacro *
eval_define_macro(Value exp, Cenv *env)
{
	if (!env->toplevel())
		syntax_error("define-macro must be non-local");
	if (!is_pair(exp))
		syntax_error("define-macro");
	Value name = car(exp);
	if (!is_pair(name))
		syntax_error("define-macro");
	if (!is_pair(exp = cdr(exp)))
		syntax_error("define-macro");
	if (!is_symbol(car(name)))
		syntax_error("define-macro: name must be a symbol");

	Cenv subenv(env, cdr(name));
	Abs *body = new Abs(subenv.arity(), eval_seq(exp, &subenv));
	return new DefineMacro(env->module(), as_symbol(car(name)), body);
}

static Expr *
eval_apply(Value exp, Cenv *env)
{
	if (is_symbol(car(exp))) {
		Symbol *name = as_symbol(car(exp));
		ModuleRef *ref = env->module()->macro_lookup(name);
		if (ref)
			return eval(apply_arglist(
				as_procedure(ref->value), cdr(exp)), env);
	}
	return new App(eval(car(exp), env), eval_seq(cdr(exp), env));
}

static Expr *
eval(Value exp, Cenv *env)
{
	if (is_symbol(exp))
		return env->lookup(as_symbol(exp));
	else if (!is_pair(exp))
		return eval_lit(exp);
	else if (car(exp) == sym_quote)
		return eval_quote(cdr(exp));
	else if (car(exp) == sym_if)
		return eval_if(cdr(exp), env);
	else if (car(exp) == sym_lambda)
		return eval_lambda(cdr(exp), env);
	else if (car(exp) == sym_begin)
		return eval_seq(cdr(exp), env);
	else if (car(exp) == sym_define)
		return eval_define(cdr(exp), env);
	else if (car(exp) == sym_define_macro)
		return eval_define_macro(cdr(exp), env);
	else
		return eval_apply(exp, env);
}

Expr *
compile(Value exp, Module *mod)
{
	Cenv env(mod);
	return eval(exp, &env);
}
