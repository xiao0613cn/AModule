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
AOptionCreate(AOption *parent)
{
	AOption *option = AOptionCreate2(parent ? &parent->children_list : NULL);
	if (option != NULL)
		option->parent = parent;
	return option;
}

AMODULE_API int
AOptionDecode(AOption **option, const char *name);

AMODULE_API AOption*
AOptionFind2(struct list_head *list, const char *name);

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
AOptionChild(AOption *option, const char *name)
{
	AOption *child = AOptionFind(option, name);
	if (child == NULL)
		return NULL;
	return child->value;
}

static inline char*
AOptionChild2(struct list_head *list, const char *name)
{
	AOption *child = AOptionFind2(list, name);
	if (child == NULL)
		return NULL;
	return child->value;
}

static inline void
AOptionClear(struct list_head *list)
{
	while (!list_empty(list)) {
		AOptionRelease(list_first_entry(list, AOption, brother_entry));
	}
}

#endif
