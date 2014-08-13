#include <stddef.h>
#include <stdint.h>
#include <gc/gc.h>

typedef intptr_t Fixnum;
typedef uintptr_t Char, TypeCode, TagCode;

class Object;
class Type;

#define TYPE_CODE_BITS 10
#define TAG_CODE_BITS 19
#define NUM_TYPE_CODES (1 << TYPE_CODE_BITS)
#define NUM_TAG_CODES (1 << TAG_CODE_BITS)
#define TYPE_CODE_MASK (NUM_TYPE_CODES-1)
#define TAG_CODE_MASK (NUM_TAG_CODES-1)

enum {
	TC_CONST, TC_BOOL, TC_TYPE, TC_FIXNUM, TC_CHAR, __TC_OBJECTS
};

enum { CONST_NIL, CONST_EOF, CONST_UNDEFINED };

class Value {
	uintptr_t val;

	explicit inline Value(uintptr_t val) : val(val) {}
public:
	inline Value(Object *obj)
		: val((uintptr_t)obj) {}
	inline Value(TypeCode typecode, TagCode tagcode)
		: val(tagcode << 13 | typecode << 3 | 0x6) {}
	inline Value()
		: val(0) {}

	static inline Value from_fixnum(Fixnum ival)
		{ return Value((uintptr_t)ival << 1 | 0x1); }
	static inline Value from_char(Char cval)
		{ return Value((uintptr_t)cval << 3 | 0x2); }
	static inline Value from_bool(bool bval)
		{ return Value(TC_BOOL, bval); }
	static inline Value from_const(TagCode tag)
		{ return Value(TC_CONST, tag); }

	inline bool _is_ptr() const
		{ return (val & 0x3) == 0x0 && val; }
	inline bool _is_fixnum() const
		{ return (val & 0x1) == 0x1; }
	inline bool _is_char() const
		{ return (val & 0x7) == 0x2; }
	inline bool is_tagged(TypeCode type) const
		{ return (val & 0x1FFF) == (type << 3 | 0x6); }
	inline bool _is_bool() const
		{ return is_tagged(TC_BOOL); }
	inline bool _is_none() const
		{ return val == 0; }

	inline Object *_as_ptr() const
		{ return (Object *)val; }
	inline Fixnum _as_fixnum() const
		{ return (Fixnum)val >> 1; }
	inline Char _as_char() const
		{ return (Char)val >> 3; }
	inline TagCode tagcode() const
		{ return val >> 13; }
	inline bool _as_bool() const
		{ return tagcode() != 0; }

	inline bool operator ==(Value that)
		{ return val == that.val; }
	inline bool operator !=(Value that)
		{ return val != that.val; }
};

class GC_object {
public:
	inline void *operator new(size_t size)
		{ return GC_MALLOC(size); }
	inline void operator delete(UNUSED void *ptr, UNUSED size_t size)
		{}
};

extern Type *type_table[NUM_TYPE_CODES];

class Object : public GC_object {
protected:
	inline Object(TypeCode typecode)
		{ init_hdr(typecode); }
	inline Object(TypeCode typecode, TagCode tagcode)
		{ init_hdr(typecode, tagcode); }
public:
	inline void init_hdr(TypeCode typecode)
		{ set_hdr(typecode << 3); }
	inline void init_hdr(TypeCode typecode, TagCode tagcode)
		{ set_hdr(tagcode << 13 | typecode << 3); }
	inline TypeCode typecode() const
		{ return hdr() >> 3 & 0x3FF; }
	inline TagCode tagcode() const
		{ return hdr() >> 13 & 0x7FFFF; }
	inline Type *type() const
		{ return type_table[typecode()]; }
	inline bool is_marked() const
		{ return (hdr() & 1) != 0; }
	inline const Object *loc() const
		{ return !is_marked() ? this : (const Object *)(hdr() & ~1); }
	inline void mark(Object *newloc)
		{ set_hdr((uintptr_t)newloc | 1); }
	template <class T> inline bool is_type() const
		{ return T::TC == typecode(); }
	template <class T> inline const T *unsafe_cast() const
		{ return (const T *) this; }
	template <class T> inline T *unsafe_cast()
		{ return (T *) this; }

#if 0
	uintptr_t _hdr;
	inline uintptr_t hdr() const { return _hdr; }
	inline void set_hdr(uintptr_t val) { _hdr = val; }
#else
	inline uintptr_t *_hdr()
		{ return (uintptr_t *)this - 1; }
	inline const uintptr_t *_hdr() const
		{ return (uintptr_t *)this - 1; }
	inline uintptr_t hdr() const
		{ return *_hdr(); }
	inline void set_hdr(uintptr_t val)
		{ *_hdr() = val; }
	inline void *operator new(size_t size) {
		void *p = GC_object::operator new(sizeof(uintptr_t) + size);
		return (uintptr_t *)p + 1;
	}
#endif
};

struct Symbol;

struct Type : Object {
	enum { TC = TC_TYPE };
	TypeCode typecode;
	Symbol *name;
	inline Type(TypeCode typecode, Symbol *name, TypeCode tc = TC)
		: Object(tc), typecode(typecode), name(name) {}
};

#define NIL (Value::from_const(CONST_NIL))
#define NULL_VALUE (Value())
#define _EOF (Value::from_const(CONST_EOF))
#define UNDEFINED (Value::from_const(CONST_UNDEFINED))
#define make_fixnum(x) (Value::from_fixnum(x))
#define make_bool(x) (Value::from_bool(!!(x)))
#define make_char(x) (Value::from_char(x))
#define _T make_bool(true)
#define _F make_bool(false)

#define is_none(x) ((x)._is_none())
#define is_nil(x) ((x) == NIL)
#define is_eof(x) ((x) == _EOF)
#define is_undefined(x) ((x) == UNDEFINED)
#define is_fixnum(x) ((x)._is_fixnum())
#define is_bool(x) ((x)._is_bool())
#define is_char(x) ((x)._is_char())

#define as_fixnum(x) ((x)._as_fixnum())
#define as_bool(x) ((x)._as_bool())
#define as_char(x) ((x)._as_char())

#define is_ptr(x) ((x)._is_ptr())
#define as_ptr(x) ((x)._as_ptr())

template<class T> static inline bool
_Value_is(Value x) { return is_ptr(x) && as_ptr(x)->is_type<T>(); }

#define _Value_is(x,T) _Value_is<T>(x)
#define _Value_as(x,T) (as_ptr(x)->unsafe_cast<T>())
