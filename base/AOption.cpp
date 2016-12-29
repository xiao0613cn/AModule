#include "stdafx.h"
#include "AModule_API.h"


AMODULE_API void
AOptionExit(AOption *option)
{
	AOptionClear(&option->children_list);

	if (!list_empty(&option->brother_entry)) {
		list_del_init(&option->brother_entry);
	}
}

AMODULE_API void
AOptionRelease(AOption *option)
{
	AOptionExit(option);
	free(option);
}

AMODULE_API void
AOptionInit(AOption *option, struct list_head *list)
{
	option->name[0] = '\0';
	option->value[0] = '\0';
	option->extend = NULL;
	INIT_LIST_HEAD(&option->children_list);

	option->parent = NULL;
	if (list != NULL) {
		list_add_tail(&option->brother_entry, list);
	} else {
		INIT_LIST_HEAD(&option->brother_entry);
	}
}

AMODULE_API AOption*
AOptionCreate2(struct list_head *list)
{
	AOption *option = (AOption*)malloc(sizeof(AOption));
	if (option != NULL)
		AOptionInit(option, list);
	return option;
}

static void
AOptionSetNameOrValue(AOption *option, const char *str, size_t len)
{
	if (!option->name[0]) {
		strncpy_sz(option->name, str, len);
	} else if (!option->value[0]) {
		strncpy_sz(option->value, str, len);
	}
}

AMODULE_API int
AOptionDecode(AOption **option, const char *name)
{
	AOption *current = AOptionCreate(NULL);
	*option = current;
	if (current == NULL)
		return -ENOMEM;

	int result = -EINVAL;
	char ident = '\0';
	int layer = 0;
	for (const char *sep = name; ; ++sep)
	{
		if ((ident != '\0') && (*sep != '\0') && (*sep != ident))
			continue;

		switch (*sep)
		{
		case '\0':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);
			if (layer == 0)
				return 0;
			result = -EINVAL;
			goto _return;

		case '{':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);

			current = AOptionCreate(current);
			if (current == NULL) {
				result = -ENOMEM;
				goto _return;
			}

			++layer;
			name = sep+1;
			break;

		case '}':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);
			if (--layer < 0) {
				result = -EINVAL;
				goto _return;
			}

			if ((current->name[0] == '\0') && list_empty(&current->children_list)) {
				AOption *empty_option = current;
				current = current->parent;
				AOptionRelease(empty_option);
			} else {
				current = current->parent;
			}

			if (layer == 0)
				return 0;
			name = sep+1;
			assert(current != NULL);
			break;

		case ',':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);
			if (layer == 0) {
				result = -EINVAL;
				goto _return;
			}

			current = AOptionCreate(current->parent);
			if (current == NULL) {
				result = -ENOMEM;
				goto _return;
			}

			name = sep+1;
			break;

		case '\"':
		case '\'':
			if (ident == '\0')
				ident = *sep;
			else
				ident = '\0';
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		case ':':
		case '=':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);
			name = sep+1;
			break;

		default:
			break;
		}
	}
_return:
	AOptionRelease(*option);
	*option = NULL;
	return result;
}

AMODULE_API AOption*
AOptionClone2(AOption *option, struct list_head *list)
{
	if (option == NULL)
		return NULL;

	AOption *current = AOptionCreate2(list);
	if (current == NULL)
		return NULL;

	strcpy_sz(current->name, option->name);
	strcpy_sz(current->value, option->value);

	AOption *pos;
	list_for_each_entry(pos, &option->children_list, AOption, brother_entry)
	{
		AOption *child = AOptionClone(pos, current);
		if (child == NULL) {
			AOptionRelease(current);
			return NULL;
		}
	}
	return current;
}

AMODULE_API AOption*
AOptionFind2(struct list_head *list, const char *name)
{
	AOption *child;
	list_for_each_entry(child, list, AOption, brother_entry)
	{
		if (_stricmp(child->name, name) == 0)
			return child;
	}
	return NULL;
}

AMODULE_API AOption*
AOptionFind3(struct list_head *list, const char *name, const char *value)
{
	AOption *child;
	list_for_each_entry(child, list, AOption, brother_entry)
	{
		if ((_stricmp(child->name, name) == 0)
		 && (_stricmp(child->value, value) == 0))
			return child;
	}
	return NULL;
}
