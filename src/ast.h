struct Expr : GC_object {
	enum ExprType {
		APP, ABS, SEQ, COND, LIT, LOCAL_REF, MODULE_REF,
		MACRO_REF, DEFINE, DEFINE_MACRO
	} type;
	Expr(ExprType type) : type(type) {}
};

struct Seq : Expr {
	unsigned count;
	Expr *expr[];
};

struct App : Expr {
	Expr *fun;
	Seq *args;
	App(Expr *fun, Seq *args) : Expr(APP), fun(fun), args(args) {}
};

struct Abs : Expr {
	int arity;
	Seq *body;
	Abs(unsigned arity, Seq *body)
		: Expr(ABS), arity(arity), body(body)  {}
};

struct Cond : Expr {
	Expr *pred, *then, *other;
	Cond(Expr *pred, Expr *then, Expr *other)
		: Expr(COND), pred(pred), then(then), other(other) {}
};

struct Lit : Expr {
	Value value;
	Lit(Value value) : Expr(LIT), value(value) {}
};

struct LocalRef : Expr {
	unsigned level  : 16;
	unsigned offset : 16;
	LocalRef(unsigned level, unsigned offset)
		: Expr(LOCAL_REF), level(level), offset(offset) {}
};

struct ModuleRef : Expr {
	Symbol *name;
	Value value;
	ModuleRef(Symbol *name, Value value)
		: Expr(MODULE_REF), name(name), value(value) {}
};

struct Define : Expr {
	Module *mod;
	Symbol *name;
	Expr *value;
	Define(Module *mod, Symbol *name, Expr *value)
		: Expr(DEFINE), mod(mod), name(name), value(value) {}
};

struct DefineMacro : Expr {
	Module *mod;
	Symbol *name;
	Abs *body;
	DefineMacro(Module *mod, Symbol *name, Abs *body)
		: Expr(DEFINE_MACRO), mod(mod), name(name), body(body) {}
};
