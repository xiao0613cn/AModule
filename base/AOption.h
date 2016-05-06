#ifndef _AOPTION_H_
#define _AOPTION_H_


//////////////////////////////////////////////////////////////////////////
typedef struct AOption {
	char        name[64];
	char        value[MAX_PATH];
	void       *extend;

	struct list_head children_list;
	struct list_head brother_entry;
	struct AOption  *parent;
} AOption;

AMODULE_API void
aoption_init(AOption *option, AOption *parent);

AMODULE_API AOption*
aoption_create(AOption *parent);

AMODULE_API long
aoption_decode(AOption **option, const char *name);

AMODULE_API AOption*
aoption_clone(AOption *option);

AMODULE_API AOption*
aoption_find_child(AOption *option, const char *name);

AMODULE_API void
aoption_release(AOption *option);


#endif
