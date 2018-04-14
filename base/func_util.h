#ifndef _FUNC_UTIL_H_
#define _FUNC_UTIL_H_


#define reset_nif(ptr, invalid, func) \
	do { \
		if ((ptr) != invalid) { \
			func; \
			(ptr) = invalid; \
		} \
	} while (0)

#define reset_s(ptr, invalid, func) reset_nif(ptr, invalid, func(ptr))
#define release_s(ptr)              reset_nif(ptr, NULL, (ptr)->release())
#define closesocket_s(sock)         reset_nif(sock, INVALID_SOCKET, closesocket(sock));

template <typename AType> AType*
addref_s(AType *&dest, AType *src) {
	if (dest != src) {
		if (dest) dest->release();
		dest = src;
		if (src) src->addref();
	}
	return dest;
}

template <typename AType> AType&
memzero(AType &stru) {
	memset(&stru, 0, sizeof(stru));
	return stru;
}


#ifndef feq
#define feq(a, b)  (fabs(a-b) < 0.000001)
#endif

#define goarrary(type, count) (type*)calloc(count, sizeof(type))
#define gomake(type)          (type*)malloc(sizeof(type))


/* godefer():
	FILE *fp = fopen(path);
	if (fp == NULL)
		return;
	godefer(FILE*, fp, fclose(fp));
	......
	fread, fwrite......
*/
#define godefer_struct(name, type, member, close) \
struct name { \
	type member; \
	name(type v) :member(v) { } \
	~name() { close; } \
	type operator->() { return member; } \
	operator type&() { return member; } \
}

#define godefer_close2(line, type, member, close) \
	godefer_struct(ac_##line##_t, type, member, close) ac_##line(member)
#define godefer_close(line, type, member, close) \
	godefer_close2(line, type, member, close)
#define godefer(type, member, close) \
	godefer_close(__LINE__, type, member, close)

#define godefer_inline2(name, line, type, member, close) \
	godefer_struct(name##line, type, member, close)
#define godefer_inline(name, line, type, member, close) \
	godefer_inline2(name, line, type, member, close)
#define godefer2(type, member, close) \
	godefer_inline(auto_close_, __LINE__, type, member, close) member


template <typename AType> int
find_ix(AType *array, AType end, AType test) {
	for (int ix = 0; array[ix] != end; ++ix) {
		if (array[ix] == test)
			return ix;
	}
	return -1;
}

template <typename AType> int
find_ix0(AType *array, AType end, AType test) {
	return (array ? find_ix<AType>(array, end, test) : 0);
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
