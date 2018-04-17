#ifndef _AOPTION_H_
#define _AOPTION_H_

typedef struct AOption AOption;

//////////////////////////////////////////////////////////////////////////
// same as cJSON ??
enum AOption_Types {
	AOption_Any = 0,
	AOption_false,
	AOption_true,
	AOption_null,
	AOption_Number,
	AOption_Double,
	AOption_String,
	AOption_StrExt, // char *extend: has "\\ \/ \b \f \n \r \t \u..."
	AOption_Array,
	AOption_Object,
};

AMODULE_API AOption*
AOptionCreate(AOption *parent, const char *name = NULL, const char *value = NULL);

AMODULE_API void
AOptionRelease(AOption *option);

AMODULE_API AOption*
AOptionFind(AOption *parent, const char *name);

AMODULE_API AOption*
AOptionFind3(AOption *parent, const char *name, const char *value);

AMODULE_API AOption*
AOptionClone(AOption *option, AOption *parent);


struct AOption {
	char      name[32];
	char      value[144];
	int       name_len;
	int       value_len;

	AOption_Types type;
	union {
	int64_t   value_i64;
	double    value_dbl;
	long      refcount;
	char     *extend;
	};

	list_head children_list;
	list_head brother_entry;
	AOption  *parent;

#ifdef __cplusplus
	// self
	long addref()  { return InterlockedAdd(&refcount, 1); }
	void delref()  { if (InterlockedAdd(&refcount, -1) == 0) release(); }
	long release() { AOptionRelease(this); return 0; }
	int  vfmt(const char *fmt, ...) {
		va_list ap; va_start(ap, fmt);
		value_len = vsnprintf(value, sizeof(value), fmt, ap);
		va_end(ap);
		return value_len;
	}
	AOption* first() { return list_entry(children_list.next, AOption, brother_entry); }
	AOption* next()  { return list_entry(brother_entry.next, AOption, brother_entry); }

	// child
	AOption* set(const char *child_name, const char *child_value) {
		return AOptionCreate(this, child_name, child_value);
	}
	AOption* fmt(const char *child_name, const char *fmt, ...) {
		AOption *child = set(child_name, NULL);
		if (child != NULL) {
			va_list ap; va_start(ap, fmt);
			child->value_len = vsnprintf(child->value, sizeof(child->value), fmt, ap);
			va_end(ap);
		}
		return child;
	}
	AOption* find(const char *child_name) {
		return AOptionFind(this, child_name);
	}
	char* getStr(const char *child_name, char *def_value) {
		AOption *child = find(child_name);
		if ((child == NULL) || (child->value[0] == '\0'))
			return def_value;
		return child->value;
	}
	int64_t getI64(const char *child_name, int64_t def_value) {
		AOption *child = find(child_name);
		if ((child == NULL) || (child->value[0] == '\0'))
			return def_value;
		return atoll(child->value);
	}
	int getInt(const char *child_name, int def_value) {
		return (int)getI64(child_name, def_value);
	}
	double getDbl(const char *child_name, double def_value) {
		AOption *child = find(child_name);
		if ((child == NULL) || (child->value[0] == '\0'))
			return def_value;
		return strtod(child->value, NULL);
	}
#endif
};

AMODULE_API void
AOptionInit(AOption *option, AOption *parent);

AMODULE_API void
AOptionExit(AOption *option);

AMODULE_API int
AOptionDecode(AOption **option, const char *name, int len);

AMODULE_API int
AOptionEncode(AOption *option, void *p, int(*write_cb)(void *p, const char *str, int len));

AMODULE_API ARefsBuf*
AOptionEncode2(AOption *option);

AMODULE_API int
AOptionLoad(AOption **option, const char *path);

AMODULE_API int
AOptionSave(AOption *option, const char *path);

#define list_for_AOption(child, parent) \
	list_for_each2(child, &(parent)->children_list, AOption, brother_entry)


#endif
