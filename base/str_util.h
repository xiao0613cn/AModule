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

#else //_WIN32

#ifndef _stricmp
#define _stricmp     strcasecmp
#endif
#ifndef _strnicmp
#define _strnicmp    strncasecmp
#endif

#endif //_WIN32

#define _strnicmp_c(ptr, c_str)  _strnicmp(ptr, c_str, sizeof(c_str)-1)

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
inline char* strncpy_sz(char (&dest)[size], const char *src, size_t len)
{
	return strncpy_sz(dest, size, src, len);
}

template <size_t size>
inline char* strcpy_sz(char (&dest)[size], const char *src)
{
	return strcpy_sz(dest, size, src);
}
#endif

#endif
