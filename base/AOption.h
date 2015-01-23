#ifndef _AOPTION_H_
#define _AOPTION_H_

#ifndef _LIST_HEAD_H_
#include "list.h"
#endif

//////////////////////////////////////////////////////////////////////////
typedef struct AOption {
	char        name[64];
	char        value[MAX_PATH];
	void       *extend;

	struct list_head children_list;
	struct list_head brother_entry;
	struct AOption  *parent;
} AOption;

extern void
AOptionInit(AOption *option, AOption *parent);

extern AOption*
AOptionCreate(AOption *parent);

extern long
AOptionDecode(AOption **option, const char *name);

extern AOption*
AOptionClone(AOption *option);

extern AOption*
AOptionFindChild(AOption *option, const char *name);

extern void
AOptionRelease(AOption *option);


#endif
