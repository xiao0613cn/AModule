#ifndef _STR_UTIL_H_
#define _STR_UTIL_H_

struct str_t {
	char *str;
	int   len;
#ifdef __cplusplus
	str_t(const char *p = NULL, int n = 0) {
		str = (char*)p; len = n;
		if (len < 0) len = strlen(str);
	}
#define sz_t(s) str_t(s, sizeof(s)-1)
#endif
};

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
#ifndef strtof
#define strtof(str, end)  (float)strtod(str, end)
#endif

#else //_WIN32

#ifndef _atoi64
#define _atoi64  atoll
#endif

#endif //!_WIN32

#define strncmp_sz(ptr, c_str)      strncmp(ptr, c_str, sizeof(c_str)-1)
#define strncasecmp_sz(ptr, c_str)  strncasecmp(ptr, c_str, sizeof(c_str)-1)


static inline char*
strncpy_sz(char *dest, size_t size, const char *src, size_t len) {
	if (src == NULL) {
		dest[0] = '\0';
	} else {
		if (len > size-1) len = size-1;
		strncpy(dest, src, len);
		dest[len] = '\0';
	}
	return dest;
}

static inline char*
strcpy_sz(char *dest, size_t size, const char *src) {
	return strncpy_sz(dest, size, src, INFINITE);
}

#ifdef __cplusplus
template <size_t size> char*
strncpy_sz(char (&dest)[size], const char *src, size_t len) {
	return strncpy_sz(dest, size, src, len);
}

template <size_t size> char*
strcpy_sz(char (&dest)[size], const char *src) {
	return strcpy_sz(dest, size, src);
}
#endif

static inline const char*
strnchr(const char *str, int val, size_t len) {
	const char *end = str + len;
	while ((str != end) && (*str != '\0')) {
		if (*str == val)
			return str;
		++str;
	}
	return NULL;
}

#define tm_fmt      "%04d-%02d-%02d %02d:%02d:%02d"
#define tm_sfmt     "%04d%02d%02d%02d%02d%02d"
#define tm_args(t)  \
	(t)->tm_year+1900, (t)->tm_mon+1, (t)->tm_mday, \
	(t)->tm_hour, (t)->tm_min, (t)->tm_sec

#define c4_args(p)  (p)[0], (p)[1], (p)[2], (p)[3]

static inline int
strtotm(const char *str, struct tm *t) {
	int result = sscanf(str, "%d-%d-%d %d:%d:%d",
		&t->tm_year, &t->tm_mon, &t->tm_mday,
		&t->tm_hour, &t->tm_min, &t->tm_sec);
	t->tm_year -= 1900;
	t->tm_mon -= 1;
	return result;
}

static inline time_t
strtotime(const char *str) {
	struct tm t = { 0 };
	strtotm(str, &t);
	return mktime(&t);
}

static inline char*
strndup(const char *src, int len) {
	char *str = (char*)malloc(len + 2);
	if (str != NULL) {
		memcpy(str, src, len);
		str[len] = '\0';
	}
	return str;
}

static inline char*
strreplace(char *str, int size, const char *param, const char *value) {
	char *ptr = strstr(str, param);
	if (ptr == NULL)
		return NULL;

	int slen = strlen(str);
	int plen = strlen(param);
	int vlen = strlen(value);

	if (size > slen+1-plen+vlen)
		size = slen+1-plen+vlen;
	else
		vlen = size-slen-1+plen;

	memmove(ptr+vlen, ptr+plen, size-1-(ptr-str)-vlen);
	memcpy(ptr, value, vlen);
	str[size-1] = '\0';
	return ptr;
}



#define begin_cmd(cmd) \
	if ((strncasecmp(buf, cmd, sizeof(cmd)-1) == 0) \
	 && (buf[sizeof(cmd)-1] == '\0' || buf[sizeof(cmd)-1] == ' ' || buf[sizeof(cmd)-1] == '\n')) { \
		begin_param(buf)

#define next_cmd(cmd) \
	end_cmd() begin_cmd(cmd)

#define end_cmd() \
		continue; }


#define begin_param(buf) \
	char *prev_param = buf;

#define next_param(name) \
	char *name = strchr(prev_param, ' '); \
	if (name == NULL) { \
		TRACE("request param: " #name "\n"); \
		continue; \
	} \
	*name++ = '\0'; \
	prev_param = name;

#define end_param() \
	char *last_param = strchr(prev_param, '\n'); \
	if (last_param != NULL) \
		 *last_param++ = '\0'; \
	prev_param = last_param;

#endif
