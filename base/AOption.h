#ifndef _AOPTION_H_
#define _AOPTION_H_


//////////////////////////////////////////////////////////////////////////
// same as cJSON ??
enum AOption_Types {
	AOption_Any = 0,
	AOption_false,
	AOption_true,
	AOption_NULL,
	AOption_Number,
	AOption_String,
	AOption_StrExt, // has "\\ \/ \b \f \n \r \t \u..."
	AOption_Array,
	AOption_Object,
};

typedef struct AOption
{
	char        name[32];
	char        value[144];
	int         name_len;
	int         value_len;

	AOption_Types type;
	union {
	int64_t     value_i64;
	double      value_dbl;
	};
	void       *extend;

	struct list_head children_list;
	struct list_head brother_entry;
	struct AOption  *parent;
} AOption;

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
	strcpy_sz(child->value, value);
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

static inline void
AOptionClear(struct list_head *list)
{
	while (!list_empty(list)) {
		AOptionRelease(list_first_entry(list, AOption, brother_entry));
	}
}

#endif
