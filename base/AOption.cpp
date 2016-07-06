#include "stdafx.h"
#include "AModule_API.h"


AMODULE_API void
AOptionRelease(AOption *option)
{
	while (!list_empty(&option->children_list)) {
		AOption *child = list_first_entry(&option->children_list, AOption, brother_entry);
		list_del_init(&child->brother_entry);
		AOptionRelease(child);
	}

	assert(list_empty(&option->brother_entry));
	free(option);
}

AMODULE_API void
AOptionInit(AOption *option, AOption *parent)
{
	option->name[0] = '\0';
	option->value[0] = '\0';
	option->extend = NULL;
	INIT_LIST_HEAD(&option->children_list);

	option->parent = parent;
	if (parent != NULL) {
		list_add_tail(&option->brother_entry, &parent->children_list);
	} else {
		INIT_LIST_HEAD(&option->brother_entry);
	}
}

AMODULE_API AOption*
AOptionCreate(AOption *parent)
{
	AOption *option = (AOption*)malloc(sizeof(AOption));
	if (option != NULL)
		AOptionInit(option, parent);
	return option;
}

static void
AOptionSetNameOrValue(AOption *option, const char *str, size_t len)
{
	if (!option->name[0]) {
		strncpy(option->name, str, max(sizeof(option->name),len));
	} else if (!option->value[0]) {
		strncpy(option->value, str, max(sizeof(option->value),len));
	}
}

AMODULE_API int
AOptionDecode(AOption **option, const char *name)
{
	AOption *current = AOptionCreate(NULL);
	if (current == NULL)
		return -ENOMEM;

	*option = current;
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
				list_del_init(&current->brother_entry);
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
AOptionClone(AOption *option)
{
	if (option == NULL)
		return NULL;

	AOption *current = AOptionCreate(NULL);
	if (current == NULL)
		return NULL;

	strcpy_s(current->name, option->name);
	strcpy_s(current->value, option->value);

	AOption *pos;
	list_for_each_entry(pos, &option->children_list, AOption, brother_entry)
	{
		AOption *child = AOptionClone(pos);
		if (child == NULL) {
			AOptionRelease(current);
			return NULL;
		}

		child->parent = current;
		list_add_tail(&child->brother_entry, &current->children_list);
	}
	return current;
}

AMODULE_API AOption*
AOptionFind(AOption *option, const char *name)
{
	if (option == NULL)
		return NULL;

	AOption *child;
	list_for_each_entry(child, &option->children_list, AOption, brother_entry)
	{
		if (_stricmp(child->name, name) == 0) {
			return child;
		}
	}
	return NULL;
}
