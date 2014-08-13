#include "lisp.h"
#include "ast.h"

Module::Module() : Object(TC)
{
	define(sym_this_module, this);
}

#define LOOKUP(sym) \
	((ModuleRef *) defs.lookup((sym)->value))

#define DEFINE(sym, val) \
	defs.define((sym)->value, val)

ModuleRef *
Module::define(Symbol *name, Value value)
{
	ModuleRef *ref = LOOKUP(name);
	if (ref && ref->value == UNDEFINED)
		ref->value = value;
	else {
		ref = new ModuleRef(name, value);
		DEFINE(name, ref);
	}
	return ref;
}

ModuleRef *
Module::lookup(Symbol *name)
{
	ModuleRef *ref = LOOKUP(name);
	if (!ref) {
		ref = new ModuleRef(name, UNDEFINED);
		DEFINE(name, ref);
	}
	return ref;
}

void
Module::undefine(Symbol *name)
{
	ModuleRef *ref = LOOKUP(name);
	if (ref && ref->value != UNDEFINED)
		DEFINE(name, new ModuleRef(name, UNDEFINED));
}

ModuleRef *
Module::macro_define(Symbol *name, Procedure *proc)
{
	ModuleRef *ref = new ModuleRef(name, proc);
	ref->type = Expr::MACRO_REF;
	DEFINE(name, ref);
	return ref;
}

ModuleRef *
Module::macro_lookup(Symbol *name)
{
	ModuleRef *ref = LOOKUP(name);
	if (ref && ref->type == Expr::MACRO_REF)
		return ref;
	return 0;
}
