#include "stdafx.h"
#include "AModule_API.h"
#include <ctype.h>


AMODULE_API void
AOptionExit(AOption *option)
{
	option->clear();
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
AOptionInit(AOption *option, AOption *parent)
{
	option->name[0] = '\0';
	option->value[0] = '\0';
	option->name_len = 0;
	option->value_len = 0;

	option->type = AOption_Any;
	option->value_i64 = 0;
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
AOptionCreate(AOption *parent, const char *name, const char *value)
{
	AOption *option = gomake(AOption);
	if (option != NULL) {
		AOptionInit(option, parent);
		strcpy_sz(option->name, name);
		strcpy_sz(option->value, value);
	}
	return option;
}

static inline void
AOptionSetKeyOrValue(AOption *current, char sep, char keysep)
{
	if (keysep == 0) {
		int max_size = _countof(current->name)-1;
		if ((current->parent != NULL) && (current->parent->type == AOption_Array))
			max_size += _countof(current->value);

		if (current->name_len < max_size)
			current->name[current->name_len++] = sep;
	} else if (keysep == 1) {
		if (current->value_len < _countof(current->value)-1)
			current->value[current->value_len++] = sep;
	}
}

static inline void
AOptionEndKeyOrValue(AOption *current, char keysep)
{
	if (keysep == 0) {
		current->name[current->name_len] = '\0';

		int max_size = _countof(current->name)-1;
		if ((current->parent != NULL) && (current->parent->type == AOption_Array))
			max_size += _countof(current->value);

		if (current->name_len >= max_size) {
			TRACE2("option(%s) maybe out of string, max size = %d!\n",
				current->name, max_size);
		}
	} else if (keysep == 1) {
		current->value[current->value_len] = '\0';
		if (current->value_len >= _countof(current->value)-1) {
			TRACE2("option(%s:%s) maybe out of string, max size = %d!\n",
				current->name, current->value, _countof(current->value));
		}
	}
}

static inline void
AOptionDecodeSlash(AOption *current, char sep, char keysep)
{
	switch (sep)
	{
	case '"': case '\\': case '/': break;
	case 'b': sep = '\b'; break;
	case 'f': sep = '\f'; break;
	case 'n': sep = '\n'; break;
	case 'r': sep = '\r'; break;
	case 't': sep = '\t'; break;
	case 'u': return;
	default: break;
	}
	AOptionSetKeyOrValue(current, sep, keysep);
}

AMODULE_API int
AOptionDecode(AOption **option, const char *name, int len)
{
	AOption *current = AOptionCreate(NULL);
	*option = current;
	if (current == NULL)
		return -ENOMEM;

	int  result  = -EINVAL;
	char ident   = '\0';
	char comment = '\0';
	char keysep  = '\0';
	char slash   = '\0';
	int  layer   = 0;

	const char *raw_str = name;
	for (const char *sep = name, *end = name+len; sep != end; ++sep)
	{
		if (*sep == '\0') {
			AOptionEndKeyOrValue(current, keysep);
			if (layer == 0)
				return (sep - raw_str);
			result = -EINVAL;
			goto _return;
		}

		if (ident != '\0') {
			if (slash != '\0') {
				AOptionDecodeSlash(current, *sep, keysep);
				slash = '\0';
				continue;
			}
			if (*sep == '\\') {
				slash = *sep;
				continue;
			}
			if (*sep != ident) {
				AOptionSetKeyOrValue(current, *sep, keysep);
				continue;
			}
		}

		if (comment != '\0') {
			if (*sep != '\n') {
				name = sep+1;
				continue;
			}
			comment = '\0';
		}

		switch (*sep)
		{
		case '{':
		case '[':
			if (sep != name) {
				AOptionEndKeyOrValue(current, keysep);
			}
			if ((*sep == '[') && (current->value[0] == '\0')) {
				current->value[0] = '[';
				current->value[1] = '\0';
				current->type = AOption_Array;
			}

			current = AOptionCreate(current);
			if (current == NULL) {
				result = -ENOMEM;
				goto _return;
			}

			++layer;
			keysep = 0;
			name = sep+1;
			break;

		case '}':
		case ']':
			if (sep != name) {
				AOptionEndKeyOrValue(current, keysep);
			}
			if (--layer < 0) {
				result = -EINVAL;
				goto _return;
			}

			if ((current->name[0] == '\0')
			 && (current->type == AOption_Any)
			 && list_empty(&current->children_list)) {
				AOption *empty_option = current;
				current = current->parent;
				AOptionRelease(empty_option);
			} else {
				current = current->parent;
			}
			if (layer == 0)
				return (sep+1 - raw_str);

			keysep = 0;
			name = sep+1;
			assert(current != NULL);
			break;

		case ',':
			if (sep != name) {
				AOptionEndKeyOrValue(current, keysep);
			}
			if (layer == 0) {
				result = -EINVAL;
				goto _return;
			}

			current = AOptionCreate(current->parent);
			if (current == NULL) {
				result = -ENOMEM;
				goto _return;
			}

			keysep = 0;
			name = sep+1;
			break;

		case '"':
		case '\'':
		case '`':
			if (ident == '\0') {
				ident = *sep;
				if ((keysep == 1) && (current->type == AOption_Any))
					current->type = AOption_String;
			} else {
				assert(ident == *sep);
				ident = '\0';
			}
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		case ':':
		case '=':
		case '#':
			if (sep != name) {
				AOptionEndKeyOrValue(current, keysep);
				keysep ++;
			}
			name = sep+1;

			if (*sep == '#')
				comment = *sep;
			break;

		default:
			AOptionSetKeyOrValue(current, *sep, keysep);
			break;
		}
	}
_return:
	AOptionRelease(*option);
	*option = NULL;
	return result;
}

AMODULE_API int
AOptionEncode(AOption *option, void *p, int(*write_cb)(void *p, const char *str, int len))
{
#define write_sn(s, n) \
	ret = write_cb(p, s, n); \
	if (ret < 0) \
		return ret; \
	write_size += n;

#define write_str(s) \
	len = strlen(s); \
	write_sn(s, len)

#define write_sc(s, i, c) \
	if (s[i] != c[0]) { \
		write_sn(c, 1) \
	}

	int write_size = 0;
	int ret, len;
	AOption *current = option;
	for (;;) {
		if (current->name[0] != '\0') {
			write_sc(current->name, 0, "\"");
			write_str(current->name);
			write_sc(current->name, len-1, "\"");
		}

		if (((current->value[0] == '\0' && current->type == AOption_Any)
		  || (current->name_len >= _countof(current->name)))
		 && current->children_list.empty()) {
			goto _next;
		}
		if (current->name[0] != '\0') {
			write_sn(":", 1);
		}

		// array
		if ((current->type == AOption_Array) || (current->value[0] == '[')) {
			write_sn("[", 1);
			if (!current->children_list.empty()) {
				current = current->first();
				continue;
			} else {
				write_sn("]", 1);
				goto _next;
			}
		}

		// value
		if ((current->type != AOption_Object) && current->children_list.empty()) {
			if ((current->type != AOption_String)
			 && (current->type != AOption_StrExt)
			 && ((current->type == AOption_false)
			  || (current->type == AOption_true)
			  || (current->type == AOption_null)
			  || (current->type == AOption_Number)
			  || (current->value[0] == '+')
			  || (current->value[0] == '-')
			  || isdigit(current->value[0])))
			{
				write_str(current->value);
			} else {
				write_sc(current->value, 0, "\"");
				if (current->type == AOption_StrExt) {
					write_str(current->extend);
				} else {
					write_str(current->value);
				}
				write_sc(current->value, len-1, "\"");
			}
			goto _next;
		}

		// object
		write_sn("{", 1);
		if (!current->children_list.empty()) {
			current = current->first();
			continue;
		}
		write_sn("}", 1);
_next:
		if ((current != option)
		 && list_is_last(&current->brother_entry, &current->parent->children_list))
		{
			current = current->parent;
			if ((current->type == AOption_Array) || (current->value[0] == '[')) {
				write_sn("]", 1);
			} else {
				write_sn("}", 1);
			}
			goto _next;
		}

		if (current != option) {
			current = list_entry(current->brother_entry.next, AOption, brother_entry);
			write_sn(",", 1);
			continue;
		}
		return write_size;
	}
#undef write_sn
#undef write_sc
#undef write_str
}

AMODULE_API int
AOptionLoad(AOption **option, const char *path)
{
	*option = NULL;
	FILE *fp = fopen(path, "rb");
	if (fp == NULL)
		return -ENOENT;
	godefer(FILE*, fp, fclose(fp));

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	if (len <= 0)
		return -EIO;

	char *buf = (char*)malloc(len+8);
	if (buf == NULL)
		return -ENOMEM;
	godefer(char*, buf, free(buf));

	fseek(fp, 0, SEEK_SET);
	int result = fread(buf, len, 1, fp);
	if (result <= 0) {
		result = -EIO;
	} else {
		result = AOptionDecode(option, buf, len);
	}
	return result;
}

static int write_buf(void *p, const char *str, int len)
{
	ARefsBuf **buf = (ARefsBuf**)p;
	int result = ARefsBuf::reserve(*buf, len, 0);
	if (result < 0)
		return result;

	(*buf)->mempush(str, len);
	return len;
}

AMODULE_API int
AOptionSave(AOption *option, const char *path)
{
	ARefsBuf *buf = NULL;
	int result = ARefsBuf::reserve(buf, 512, 0);
	if (result < 0)
		return result;
	godefer(ARefsBuf*, buf, buf->release());

	result = AOptionEncode(option, &buf, write_buf);
	if (result < 0)
		return result;

	FILE *fp = fopen(path, "wb");
	if (fp == NULL)
		return -errno;
	godefer(FILE*, fp, fclose(fp));

	result = buf->len();
	fwrite(buf->ptr(), result, 1, fp);

	return result;
}

AMODULE_API AOption*
AOptionClone(AOption *option, AOption *parent)
{
	if (option == NULL)
		return NULL;

	AOption *current = AOptionCreate(parent, NULL, NULL);
	if (current == NULL)
		return NULL;

	strcpy_sz(current->name, option->name);
	strcpy_sz(current->value, option->value);
	current->name_len = option->name_len;
	current->value_len = option->value_len;
	current->type = option->type;
	current->value_i64 = option->value_i64;

	list_for_AOption(pos, option)
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
AOptionFind(AOption *parent, const char *name)
{
	if (parent == NULL)
		return NULL;
	list_for_AOption(child, parent)
	{
		if (strcasecmp(child->name, name) == 0)
			return child;
	}
	return NULL;
}

AMODULE_API AOption*
AOptionFind3(AOption *parent, const char *name, const char *value)
{
	if (parent == NULL)
		return NULL;
	list_for_AOption(child, parent)
	{
		if ((strcasecmp(child->name, name) == 0)
		 && (strcasecmp(child->value, value) == 0))
			return child;
	}
	return NULL;
}
