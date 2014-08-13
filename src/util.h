#ifndef SRC_UTIL_H
#define SRC_UTIL_H

#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

/* pointer to statically-allocated memory? */
static inline bool
is_static(const void *p)
{
	extern const char __executable_start[], _end[];
	return (const char *) p >= __executable_start
	    && (const char *) p < _end;
}

/* exception-safe wrapper for malloc'd pointers */
template <class T>
class malloc_ptr {
	T *p;
public:
	malloc_ptr(T *p) : p(p) {}
	~malloc_ptr() { free(p); }
	operator T *() const { return p; }
};

#endif /* SRC_UTIL_H */
