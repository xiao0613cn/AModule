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
AOptionInit(AOption *option, AOption *parent);

AMODULE_API AOption*
AOptionCreate(AOption *parent);

AMODULE_API int
AOptionDecode(AOption **option, const char *name);

AMODULE_API AOption*
AOptionClone(AOption *option);

AMODULE_API AOption*
AOptionFind(AOption *option, const char *name);

static inline char*
AOptionChild(AOption *option, const char *name)
{
	AOption *child = AOptionFind(option, name);
	if (child == NULL)
		return NULL;
	return child->value;
}

AMODULE_API void
AOptionRelease(AOption *option);


#endif
