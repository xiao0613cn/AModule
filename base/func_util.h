#ifndef _AMODULE_UTIL_TOOLS_H_
#define _AMODULE_UTIL_TOOLS_H_

#ifndef release_s
#define release_s(ptr, release, null) \
	do { \
		if ((ptr) != null) { \
			release(ptr); \
			(ptr) = null; \
		} \
	} while (0)
#endif

#ifndef release_f
#define release_f(ptr, null, func) \
	do { \
		if (ptr != null) { \
			func; \
			ptr = null; \
		} \
	} while (0)
#endif

#define defer_struct(name, type, member, close) \
struct name { \
	type member; \
	name(type v) :member(v) { } \
	~name() { close; } \
	type operator->() { return member; } \
	operator type&() { return member; } \
}

#define defer_inline2(name, line, type, member, close) \
	defer_struct(name##line, type, member, close)

#define defer_inline(name, line, type, member, close) \
	defer_inline2(name, line, type, member, close)

#define defer2(type, member, close) \
	defer_inline(auto_close_, __LINE__, type, member, close) member

#define defer(type, member, close) \
	defer_inline(auto_close_, __LINE__, type, member, close) ac##__LINE__(member)

template <typename type_t>
static inline int find_ix(type_t *array, type_t end, type_t test)
{
	for (int ix = 0; array[ix] != end; ++ix) {
		if (array[ix] == test)
			return ix;
	}
	return -1;
}

template <typename type_t>
static inline int find_ix0(type_t *array, type_t end, type_t test)
{
	return (array ? find_ix<type_t>(array, end, test) : 0);
}



#endif
