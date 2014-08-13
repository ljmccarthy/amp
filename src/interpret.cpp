#include "lisp.h"
#include "ast.h"

/* hack hack hack */
enum Mode {
	APP = Expr::APP,
	ABS = Expr::ABS,
	SEQ = Expr::SEQ,
	COND = Expr::COND,
	LIT = Expr::LIT,
	LOCAL_REF = Expr::LOCAL_REF,
	MODULE_REF = Expr::MODULE_REF,
	MACRO_REF = Expr::MACRO_REF,
	DEFINE = Expr::DEFINE,
	DEFINE_MACRO = Expr::DEFINE_MACRO,
	EXIT,  // exit interpreter loop
	THEN,  // receives predicate from COND
	SEQ_NEXT,
	APP_FUN, // recieves evaluated function
	APP_ARGS,
	APP_VARGS
};

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

#define STACK_SIZE 4096

static uintptr_t stack[STACK_SIZE], *sp = stack;

static void
push(uintptr_t x)
{
	if (sp >= stack+STACK_SIZE)
		error(Error(), "stack overflow");
	*sp++ = x;
}

#define PUSH(x) push((uintptr_t)(x))
#define POP(type) ((type)(*--sp))

Value
interpret(Expr *expr, Frame *env)
{
	Mode mode = (Mode) expr->type;
	Value value = NIL;
	int count = 0;
	sp = stack;

	while (1) switch (mode) {
	case LIT:
		value = ((Lit *)expr)->value;
		goto _leave;
	case LOCAL_REF: {
		LocalRef *lref = (LocalRef *) expr;
		value = fetch(env, lref->level, lref->offset);
		goto _leave;
	}
	case MACRO_REF:
	case MODULE_REF: {
		ModuleRef *mref = (ModuleRef *) expr;
		if (mref->value == UNDEFINED)
			unbound_error(mref->name);
		value = mref->value;
		goto _leave;
		}
	case COND: {
		Cond *cond = (Cond *) expr;
		PUSH(expr);
		PUSH(THEN);
		expr = cond->pred;
		mode = (Mode) expr->type;
		break;
		}
	case THEN: {
		Cond *cond = (Cond *) expr;
		expr = (value != _F) ? cond->then : cond->other;
		mode = (Mode) expr->type;
		break;
		}
	case SEQ: {
		Seq *seq = (Seq *) expr;
		PUSH(count);
		count = seq->count;
		mode = SEQ_NEXT;
		break;
		}
	case SEQ_NEXT: {
		Seq *seq = (Seq *) expr;
		if (count == 0) {
			count = POP(int);
			goto _leave;
		}
		if (count != 1) {
			PUSH(expr);
			PUSH(SEQ_NEXT);
		}
		expr = seq->expr[seq->count - count];
		mode = (Mode) expr->type;
		count--;
		break;
		}
	case APP: {
		App *app = (App *) expr;
		PUSH(count);
		PUSH(expr);
		PUSH(APP_FUN);
		count = app->args->count;
		break;
		}
	case APP_FUN: {
		if (!is_procedure(value))
			fun_type_error();
		mode = count >= 0 ? APP_ARGS : APP_VARGS;
		}
	case APP_ARGS: {
		}
	case EXIT:
		return value;
	_leave:
		mode = POP(Mode);
		expr = POP(Expr *);
	}
	return value;
}
