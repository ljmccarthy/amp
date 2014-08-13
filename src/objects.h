/*
  Object type definitions
*/

struct Expr;
struct Frame;
struct ModuleRef;
struct GenericTable;

enum {
	TC_PAIR = __TC_OBJECTS, TC_STRING, TC_SYMBOL, TC_MODULE,
	TC_AST_PROCEDURE, TC_C_PROCEDURE, TC_CC_PROCEDURE, TC_RECORD_TYPE,
	TC_GENERIC, TC_USER
};

struct Pair : Object {
	enum { TC = TC_PAIR };
	Value fst, snd;
	inline Pair(Value fst, Value snd)
		: Object(TC), fst(fst), snd(snd) {}
};

struct String : Object {
	enum { TC = TC_STRING };
	size_t len;
	const char *value;
	String(const char *str, size_t len, TypeCode typecode = TC);
};

struct Symbol : String {
	enum { TC = TC_SYMBOL };
	inline Symbol(const char *str, size_t len)
		: String(str, len, TC) {}
};

class Dict {
	struct tstnode_t *root;
public:
	Dict() : root(0) {}
	void define(const char *key, const void *value);
	void *lookup(const char *key);
};

struct Procedure : Object {
	Symbol *name;
	int arity;
	inline Procedure(Symbol *name, int arity, TypeCode tc)
		: Object(tc), name(name), arity(arity) {}
};

struct AstProcedure : Procedure {
	enum { TC = TC_AST_PROCEDURE };
	Expr *body;
	Frame *env;
	inline AstProcedure(Symbol *name, int arity, Expr *body, Frame *env)
		: Procedure(name, arity, TC), body(body), env(env) {}
};

struct CProcedure : Procedure {
	enum { TC = TC_C_PROCEDURE };
	typedef Value proc_type(unsigned, Value *);
	proc_type *proc;
	inline CProcedure(Symbol *name, int arity, proc_type *proc)
		: Procedure(name, arity, TC), proc(proc) {}
};

struct CCProcedure : Procedure {
	enum { TC = TC_CC_PROCEDURE };
	typedef Value proc_type(void *, unsigned, Value *);
	proc_type *proc;
	inline CCProcedure(Symbol *name, int arity, proc_type *proc)
		: Procedure(name, arity, TC), proc(proc) {}
};

struct Module : Object {
	enum { TC = TC_MODULE };
	Dict defs;
	Module();
	ModuleRef *define(Symbol *name, Value value);
	ModuleRef *lookup(Symbol *name);
	void undefine(Symbol *name);
	ModuleRef *macro_define(Symbol *name, Procedure *proc);
	ModuleRef *macro_lookup(Symbol *name);
};

struct RecordType : Type {
	enum { TC = TC_RECORD_TYPE };
	Value slots;
	unsigned nslots;
	Procedure *_cons;
	RecordType(TypeCode typecode, Symbol *name, Value slots);
	Procedure *constructor();
	Procedure *accessor(Symbol *slot);
	Procedure *mutator(Symbol *slot);
};

struct Generic : Procedure {
	enum { TC = TC_GENERIC };
private:
	unsigned darity;
	GenericTable *table;
	void *default_value;
public:
	inline Generic(int darity, Symbol *name, int arity)
		: Procedure(name, arity, TC), darity(darity) {}
	void insert(uint16_t *types, void *ptr);
	void *lookup(uint16_t *types);
};

#define make_string(value, len) new String(value, len)
#define make_c_procedure(name, arity, fun) \
	new CProcedure(name, arity, fun)
#define make_ast_procedure(name, arity, body, env) \
	new AstProcedure(name, arity, body, env)

#define is_pair(x) _Value_is(x, Pair)
#define is_string(x) _Value_is(x, String)
#define is_symbol(x) _Value_is(x, Symbol)
#define is_module(x) _Value_is(x, Module)
#define is_procedure(x) (is_ast_procedure(x) || is_c_procedure(x) \
                         || is_cc_procedure(x))
#define is_c_procedure(x) _Value_is(x, CProcedure)
#define is_ast_procedure(x) _Value_is(x, AstProcedure)
#define is_cc_procedure(x) _Value_is(x, CCProcedure)
#define is_type(x) (_Value_is(x, Type) || is_record_type(x))
#define is_record_type(x) _Value_is(x, RecordType)

#define as_pair(x) _Value_as(x, Pair)
#define as_string(x) _Value_as(x, String)
#define as_symbol(x) _Value_as(x, Symbol)
#define as_module(x) _Value_as(x, Module)
#define as_procedure(x) _Value_as(x, Procedure)
#define as_c_procedure(x) _Value_as(x, CProcedure)
#define as_ast_procedure(x) _Value_as(x, AstProcedure)
#define as_cc_procedure(x) _Value_as(x, CCProcedure)
#define as_type(x) _Value_as(x, Type)
#define as_record_type(x) _Value_as(x, RecordType)

#define cons(x,y) (new Pair(x,y))
#define car(x) (as_pair(x)->fst)
#define cdr(x) (as_pair(x)->snd)
