#include "stdafx.h"
#include "AOption.h"


//////////////////////////////////////////////////////////////////////////
void AOptionRelease(AOption *option)
{
	while (!list_empty(&option->children_list)) {
		AOption *child = list_first_entry(&option->children_list, AOption, brother_entry);
		list_del_init(&child->brother_entry);
		AOptionRelease(child);
	}
	assert(list_empty(&option->brother_entry));
	free(option);
}

void AOptionInit(AOption *option, AOption *parent)
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

AOption* AOptionCreate(AOption *parent)
{
	AOption *option = (AOption*)malloc(sizeof(AOption));
	if (option != NULL)
		AOptionInit(option, parent);
	return option;
}

void AOptionSetNameOrValue(AOption *option, const char *str, size_t len)
{
	if (!option->name[0])
		strncpy_s(option->name, str, len);
	else if (!option->value[0])
		strncpy_s(option->value, str, len);
}

extern long AOptionDecode(AOption **option, const char *name)
{
	AOption *current = AOptionCreate(NULL);
	if (current == NULL)
		return -ERROR_OUTOFMEMORY;
	*option = current;

	char ident = '\0';
	long layer = 0;
	for (const char *sep = name; ; ++sep)
	{
		if ((ident != '\0') && (*sep != '\0') && (*sep != ident))
			continue;
		switch (*sep)
		{
		case '\0':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);
			return (layer ? -ERROR_INVALID_PARAMETER : 0);

		case '{':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);

			current = AOptionCreate(current);
			if (current == NULL)
				return -ERROR_OUTOFMEMORY;

			++layer;
			name = sep+1;
			break;

		case '}':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);

			if (--layer < 0)
				return -ERROR_INVALID_PARAMETER;
			if (layer == 0)
				return 0;

			current = current->parent;
			name = sep+1;
			break;

		case ',':
			if (sep != name)
				AOptionSetNameOrValue(current, name, sep-name);

			if (layer == 0)
				return -ERROR_INVALID_PARAMETER;

			current = AOptionCreate(current->parent);
			if (current == NULL)
				return -ERROR_OUTOFMEMORY;

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
}

AOption* AOptionClone(AOption *option)
{
	AOption *current = AOptionCreate(NULL);
	strcpy_s(current->name, option->name);
	strcpy_s(current->value, option->value);

	AOption *pos;
	list_for_each_entry(pos, &option->children_list, AOption, brother_entry)
	{
		AOption *child = AOptionClone(pos);
		child->parent = current;
		list_add(&child->brother_entry, &current->children_list);
	}
	return current;
}

AOption* AOptionFindChild(AOption *option, const char *name)
{
	AOption *child;
	list_for_each_entry(child, &option->children_list, AOption, brother_entry)
	{
		if (_stricmp(child->name, name) == 0) {
			return child;
		}
	}
	return NULL;
}
