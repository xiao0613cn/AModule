#ifndef _AOPTION_H_
#define _AOPTION_H_


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

typedef struct AOption AOption;
struct AOption {
	char        name[32];
	char        value[144];
	int         name_len;
	int         value_len;

	AOption_Types type;
	union {
	int64_t     value_i64;
	double      value_dbl;
	char       *extend;
	};

	struct list_head children_list;
	struct list_head brother_entry;
	struct AOption  *parent;
#ifdef __cplusplus
	// child
	AOption* create(const char *child_name = NULL);
	AOption* set(const char *child_name, const char *child_value); // child_name != NULL
	AOption* fmt(const char *child_name, const char *fmt, ...);
	AOption* find(const char *child_name);
	char*    getStr(const char *child_name, char *def_value = NULL);
	int64_t  getI64(const char *child_name, int64_t def_value = 0);
	int      getInt(const char *child_name, int def_value = 0) {
		return (int)getI64(child_name, def_value);
	}

	// self
	int      sfmt(const char *fmt, ...);
#endif
};

AMODULE_API void
AOptionInit(AOption *option, struct list_head *list);

AMODULE_API void
AOptionExit(AOption *option);

AMODULE_API AOption*
AOptionCreate2(struct list_head *list);

static inline AOption*
AOptionCreate(AOption *parent, const char *name = NULL, const char *value = NULL)
{
	AOption *option = AOptionCreate2(parent ? &parent->children_list : NULL);
	if (option != NULL) {
		option->parent = parent;
		if (name != NULL)
			strcpy_sz(option->name, name);
		if (value != NULL)
			strcpy_sz(option->value, value);
	}
	return option;
}

AMODULE_API int
AOptionDecode(AOption **option, const char *name, int len);

AMODULE_API int
AOptionEncode(const AOption *option, void *p, int(*write_cb)(void *p, const char *str, int len));

AMODULE_API int
AOptionLoad(AOption **option, const char *path);

AMODULE_API int
AOptionSave(const AOption *option, const char *path);


AMODULE_API AOption*
AOptionFind2(struct list_head *list, const char *name);

AMODULE_API AOption*
AOptionFind3(struct list_head *list, const char *name, const char *value);

static inline AOption*
AOptionFind(AOption *option, const char *name)
{
	if (option == NULL)
		return NULL;
	return AOptionFind2(&option->children_list, name);
}

AMODULE_API AOption*
AOptionClone2(AOption *option, struct list_head *list);

static inline AOption*
AOptionClone(AOption *option, AOption *parent)
{
	AOption *child = AOptionClone2(option, parent ? &parent->children_list : NULL);
	if (child != NULL)
		child->parent = parent;
	return child;
}

AMODULE_API void
AOptionRelease(AOption *option);

static inline char*
AOptionGet(AOption *option, const char *name, char *def_value = NULL)
{
	AOption *child = AOptionFind(option, name);
	if ((child == NULL) || (child->value[0] == '\0'))
		return def_value;
	return child->value;
}

static inline char*
AOptionGet2(AOption *option, const char *name, char *def_value = NULL)
{
	AOption *child = AOptionFind(option, name);
	if (child == NULL)
		return def_value;
	return child->value;
}

static inline int64_t
AOptionGetI64(AOption *option, const char *name, int64_t def_value)
{
	AOption *child = AOptionFind(option, name);
	if ((child == NULL) || (child->value[0] == '\0'))
		return def_value;
	return atoll(child->value);
}

#define AOptionGetInt(opt, name, def)  (int)AOptionGetI64(opt, name, def)

static inline char*
AOptionGet2(struct list_head *list, const char *name, char *def_value = NULL)
{
	AOption *child = AOptionFind2(list, name);
	if (child == NULL)
		return def_value;
	return child->value;
}

static inline AOption*
AOptionSet2(struct list_head *list, const char *name, const char *value)
{
	AOption *child = AOptionFind2(list, name);
	if (child == NULL) {
		child = AOptionCreate2(list);
		if (child == NULL)
			return NULL;
		strcpy_sz(child->name, name);
	}
	if (value != NULL) {
		strcpy_sz(child->value, value);
		child->type = AOption_String;
	}
	return child;
}

static inline AOption*
AOptionSet(AOption *options, const char *name, const char *value)
{
	AOption *child = AOptionSet2(&options->children_list, name, value);
	if (child != NULL)
		child->parent = options;
	return child;
}

static inline AOption*
AOptionFmt(AOption *options, const char *name, const char *fmt, ...)
{
	AOption *child = AOptionSet(options, name, NULL);
	if (child != NULL) {
		child->parent = options;
		//child->type = AOption_String;

		va_list ap;
		va_start(ap, fmt);
		child->value_len = vsnprintf(child->value, sizeof(child->value), fmt, ap);
		va_end(ap);
	}
	return child;
}

static inline void
AOptionClear(struct list_head *list)
{
	while (!list_empty(list)) {
		AOptionRelease(list_first_entry(list, AOption, brother_entry));
	}
}

#ifdef __cplusplus
inline AOption* AOption::create(const char *child_name) {
	return AOptionCreate(this, child_name);
}
inline AOption* AOption::set(const char *child_name, const char *child_value) { // child_name != NULL
	return AOptionSet(this, child_name, child_value);
}
inline AOption* AOption::fmt(const char *child_name, const char *fmt, ...) {
	AOption *child = AOptionSet(this, child_name, NULL);
	if (child != NULL) {
		child->parent = this;
		//child->type = AOption_String;
		va_list ap;
		va_start(ap, fmt);
		child->value_len = vsnprintf(child->value, sizeof(child->value), fmt, ap);
		va_end(ap);
	}
	return child;
}
inline AOption* AOption::find(const char *child_name) {
	return AOptionFind(this, child_name);
}
inline char* AOption::getStr(const char *child_name, char *def_value) {
	return AOptionGet(this, child_name, def_value);
}
inline int64_t AOption::getI64(const char *child_name, int64_t def_value) {
	return AOptionGetI64(this, child_name, def_value);
}
inline int AOption::sfmt(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	value_len = vsnprintf(value, sizeof(value), fmt, ap);
	va_end(ap);
	return value_len;
}
#endif
//////////////////////////////////////////////////////////////////////////
#define cfgInt(options, name, def_value) \
	name = AOptionGetInt(options, #name, def_value)

#define cfgStr(options, name, def_value) \
	name = AOptionGet(options, #name, def_value)

#define cfgStr2(options, name, def_value) \
	name = AOptionGet2(options, #name, def_value)

#endif
