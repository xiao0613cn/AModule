#ifndef _FUNC_UTIL_H_
#define _FUNC_UTIL_H_


#define release_s(ptr, release, null) \
	do { \
		if ((ptr) != null) { \
			release(ptr); \
			(ptr) = null; \
		} \
	} while (0)

#define release_f(ptr, null, func) \
	do { \
		if (ptr != null) { \
			func; \
			ptr = null; \
		} \
	} while (0)


#define gomake2(type, count) (type*)malloc(sizeof(type)*(count))
#define gomake(type)         (type*)malloc(sizeof(type))


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

#define defer_close2(line, type, member, close) \
	defer_struct(ac_##line##_t, type, member, close) ac_##line(member)

#define defer_close(line, type, member, close) \
	defer_close2(line, type, member, close)

#define godefer(type, member, close) \
	defer_close(__LINE__, type, member, close)


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
