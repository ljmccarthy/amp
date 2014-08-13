#include <cstring>
#include <cassert>
#include "lisp.h"

Type *type_table[NUM_TYPE_CODES];

#define init_type(code, name) \
type_table[code] = new Type(code, make_symbol(name))

void
init_types(void)
{
	init_type(TC_CONST, "const");
	init_type(TC_BOOL, "bool");
	init_type(TC_TYPE, "type");
	init_type(TC_FIXNUM, "fixnum");
	init_type(TC_CHAR, "char");
	init_type(TC_STRING, "string");
	init_type(TC_SYMBOL, "symbol");
	init_type(TC_AST_PROCEDURE, "ast-procedure");
	init_type(TC_C_PROCEDURE, "c-procedure");
	init_type(TC_CC_PROCEDURE, "cc-procedure");

	type_table[TC_PAIR] = new RecordType(TC_PAIR,
		make_symbol("pair"),
		cons(make_symbol("car"),
		     cons(make_symbol("cdr"), NIL)));
}

TypeCode
alloc_type_code(void)
{
	static TypeCode tc = TC_USER;
	return tc++;
	/* TODO: bit-tree type id allocator */
}

RecordType::RecordType(TypeCode typecode, Symbol *name, Value slots)
	: Type(typecode, name, TC), slots(slots), nslots(length(slots)),
	_cons(0)
{
	type_table[typecode] = this;
	/*
	  TODO:
	  check for non-nil slot list
	  check all slots names are unique
	  type extension
	*/
}

struct Record : Object {
	Value slots[];
};

#define as_product_value(x) _Value_as(x, Record)

static CCProcedure::proc_type constructor, accessor, mutator;

struct Constructor : CCProcedure {
	RecordType *type;
	inline Constructor(RecordType *type)
		: CCProcedure(sym_at_constructor, type->nslots, constructor),
		type(type) {}
};

struct Accessor : CCProcedure {
	TypeCode typecode;
	unsigned index;
	inline Accessor(TypeCode typecode, unsigned index)
		: CCProcedure(sym_at_accessor, 1, accessor),
		typecode(typecode), index(index) {}
};

struct Mutator : CCProcedure {
	TypeCode typecode;
	unsigned index;
	inline Mutator(TypeCode typecode, unsigned index)
		: CCProcedure(sym_at_mutator, 2, mutator),
		typecode(typecode), index(index) {}
};

static Value
constructor(void *_self, UNUSED unsigned nargs, Value *args)
{
	Constructor *self = (Constructor *)_self;

	assert(nargs == self->type->nslots);
	size_t slotsize = self->type->nslots * sizeof(Value);
	Record *p = (Record *) Object::operator new(
		sizeof(Record) + slotsize);
	p->init_hdr(self->type->typecode);
	memcpy(p->slots, args, slotsize);
	return p;
}

static Value
accessor(void *_self, UNUSED unsigned nargs, Value *args)
{
	Accessor *self = (Accessor *)_self;

	if (!is_ptr(args[0]) || as_ptr(args[0])->typecode() != self->typecode)
		type_error(self->name->value);
	return as_product_value(args[0])->slots[self->index];
}

static Value
mutator(void *_self, UNUSED unsigned nargs, Value *args)
{
	Mutator *self = (Mutator *)_self;

	if (!is_ptr(args[0]) || as_ptr(args[0])->typecode() != self->typecode)
		type_error(self->name->value);
	as_product_value(args[0])->slots[self->index] = args[1];
	return NIL;
}

Procedure *
RecordType::constructor()
{
	return _cons ? _cons : (_cons = new Constructor(this));
}

/* TODO: cache accessor */
Procedure *
RecordType::accessor(Symbol *slot)
{
	int index = memq_index(slot, slots);
	if (index < 0)
		errorf(Error(), "slot not found: %s", slot->value);
	return new Accessor(typecode, index);
}

/* TODO: cache mutator */
Procedure *
RecordType::mutator(Symbol *slot)
{
	int index = memq_index(slot, slots);
	if (index < 0)
		errorf(Error(), "slot not found: %s", slot->value);
	return new Mutator(typecode, index);
}
