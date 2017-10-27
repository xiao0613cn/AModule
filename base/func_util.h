#ifndef _FUNC_UTIL_H_
#define _FUNC_UTIL_H_


#define if_not2(ptr, invalid, func) \
	do { \
		if ((ptr) != invalid) { \
			func; \
			(ptr) = invalid; \
		} \
	} while (0)

#define if_not(ptr, invalid, func)  if_not2(ptr, invalid, func(ptr));
#define release_s(ptr)              if_not2(ptr, NULL, (ptr)->release())
#define closesocket_s(sock)         if_not2(sock, INVALID_SOCKET, closesocket(sock));

/*
template <typename object_t>
struct ARefsPtr {
	object_t *_this;

	void init(object_t *p = NULL) { _this = p; }
	object_t *operator->() { return _this; }
	operator object_t*() { return _this; }

	object_t* operator=(object_t *p) {
		if (_this != NULL) _this->release();
		_this = p;
		return _this;
	}
	object_t* operator=(ARefsPtr<object_t> &other) {
		if (_this != NULL) _this->release();
		_this = other._this;
		if (_this != NULL) _this->addref();
		return _this;
	}
};
*/

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

#define defer_close2(line, type, member, close) \
	defer_struct(ac_##line##_t, type, member, close) ac_##line(member)
#define defer_close(line, type, member, close) \
	defer_close2(line, type, member, close)
#define godefer(type, member, close) \
	defer_close(__LINE__, type, member, close)

#define defer_inline2(name, line, type, member, close) \
	defer_struct(name##line, type, member, close)
#define defer_inline(name, line, type, member, close) \
	defer_inline2(name, line, type, member, close)
#define godefer2(type, member, close) \
	defer_inline(auto_close_, __LINE__, type, member, close) member


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


/* file: minunit.h */
#define mu_assert(message, test) \
	do { if (!(test)) return message; } while (0)

#define mu_run_test(test) \
	do { char *message = test(); tests_run++; \
	     if (message) return message; } while (0)

extern int tests_run;
/* file: minunit.h */


#endif
