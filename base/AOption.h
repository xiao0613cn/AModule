#ifndef _AOPTION_H_
#define _AOPTION_H_


//////////////////////////////////////////////////////////////////////////
typedef struct AOption
{
	char        name[64];
	char        value[BUFSIZ];
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
AOptionDecode(AOption **option, const char *name);

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

static inline const char*
AOptionGet(AOption *option, const char *name, const char *def_value = NULL)
{
	AOption *child = AOptionFind(option, name);
	if ((child == NULL) || (child->value[0] == '\0'))
		return def_value;
	return child->value;
}

static inline const char*
AOptionGet2(AOption *option, const char *name, const char *def_value = NULL)
{
	AOption *child = AOptionFind(option, name);
	if (child == NULL)
		return def_value;
	return child->value;
}

static inline int
AOptionGetInt(AOption *option, const char *name, int def_value = 0)
{
	AOption *child = AOptionFind(option, name);
	if ((child == NULL) || (child->value[0] == '\0'))
		return def_value;
	return atoi(child->value);
}

static inline char*
AOptionGet2(struct list_head *list, const char *name, char *def_value = NULL)
{
	AOption *child = AOptionFind2(list, name);
	if (child == NULL)
		return def_value;
	return child->value;
}

static inline int
AOptionSet2(struct list_head *list, const char *name, const char *value)
{
	AOption *child = AOptionFind2(list, name);
	if (child == NULL) {
		child = AOptionCreate2(list);
		if (child == NULL)
			return -ENOMEM;
		strcpy_sz(child->name, name);
	}
	strcpy_sz(child->value, value);
}

static inline void
AOptionClear(struct list_head *list)
{
	while (!list_empty(list)) {
		AOptionRelease(list_first_entry(list, AOption, brother_entry));
	}
}

#endif
