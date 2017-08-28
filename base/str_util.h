#ifndef _STR_UTIL_H_
#define _STR_UTIL_H_


#ifndef _tostring
#define _tostring(x) #x
#endif

#ifdef _WIN32

#ifndef snprintf
#define snprintf  _snprintf
#endif
#ifndef strcasecmp
#define strcasecmp  _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp  _strnicmp
#endif
#ifndef atoll
#define atoll  _atoi64
#endif

#else //_WIN32

#ifndef _atoi64
#define _atoi64  atoll
#endif

#endif //_WIN32

#define strncmp_sz(ptr, c_str)   strncmp(ptr, c_str, sizeof(c_str)-1)
#define strncasecmp_sz(ptr, c_str)  strncasecmp(ptr, c_str, sizeof(c_str)-1)


static inline char*
strncpy_sz(char *dest, size_t size, const char *src, size_t len)
{
	if (src == NULL) {
		dest[0] = '\0';
	} else {
		if (len > size-1)
			len = size-1;
		strncpy(dest, src, len);
		dest[len] = '\0';
	}
	return dest;
}

static inline char*
strcpy_sz(char *dest, size_t size, const char *src)
{
	return strncpy_sz(dest, size, src, 0xffffffff);
}

#ifdef __cplusplus
template <size_t size>
inline char*
strncpy_sz(char (&dest)[size], const char *src, size_t len)
{
	return strncpy_sz(dest, size, src, len);
}

template <size_t size>
inline char*
strcpy_sz(char (&dest)[size], const char *src)
{
	return strcpy_sz(dest, size, src);
}
#endif

static inline const char*
strnchr(const char *str, int val, size_t len)
{
	const char *end = str + len;
	while ((str != end) && (*str != '\0')) {
		if (*str == val)
			return str;
		++str;
	}
	return NULL;
}

#endif
