#include "stdafx.h"
#include "../base/AModule.h"

struct DumpObject {
	AObject  object;
	HANDLE   file;
	AObject *io;
	BYTE     req_list[32];
};
#define to_dump(obj)   container_of(obj, DumpObject, object)

static void DumpRelease(AObject *object)
{
	DumpObject *dump = to_dump(object);
	release_s(dump->file, CloseHandle, INVALID_HANDLE_VALUE);
	release_s(dump->io, AObjectRelease, NULL);

	free(dump);
}

static long DumpCreate(AObject **object, AObject *parent, AOption *option)
{
	DumpObject *dump = (DumpObject*)malloc(sizeof(DumpObject));
	if (dump == NULL)
		return -ENOMEM;

	extern AModule DumpModule;
	AObjectInit(&dump->object, &DumpModule);
	dump->file = INVALID_HANDLE_VALUE;
	dump->io = NULL;
	memset(dump->req_list, 0, sizeof(dump->req_list));

	AOption *io_opt = AOptionFindChild(option, "io");
	if (io_opt != NULL)
		AObjectCreate(&dump->io, &dump->object, io_opt, NULL);

	*object = &dump->object;
	return 1;
}

static long DumpOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AOption)))
		return -EINVAL;

	DumpObject *dump = to_dump(object);
	AOption *opt = AOptionFindChild((AOption*)msg->data, "file");

	release_s(dump->file, CloseHandle, INVALID_HANDLE_VALUE);
	if ((opt->value[0] != '\0') && (opt->value[1] != '\0'))
	{
		dump->file = CreateFileA(opt->value, GENERIC_WRITE, FILE_SHARE_READ,
		                         NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (dump->file == INVALID_HANDLE_VALUE) {
			TRACE("CreateFile(%s) error = %d.\n", opt->value, GetLastError());
			return -EBADF;
		}
	}

	opt = AOptionFindChild((AOption*)msg->data, "req_list");
	if (opt != NULL) {
		memset(dump->req_list, 0, sizeof(dump->req_list));
		const char *sep = opt->value;
		do {
			long reqix = atol(sep);
			if (reqix < _countof(dump->req_list))
				dump->req_list[reqix] = TRUE;
		} while (((sep=strchr(sep,',')) != NULL) && (*++sep != '\0'));
	} else {
		memset(dump->req_list, TRUE, sizeof(dump->req_list));
	}

	opt = AOptionFindChild((AOption*)msg->data, "io");
	if (dump->io == NULL) {
		AObjectCreate(&dump->io, &dump->object, opt, NULL);
		if (dump->io == NULL)
			return -ENXIO;
	}
	msg->data = (char*)opt;
	return dump->io->open(dump->io, msg);
}

static long DumpRequest(AObject *object, long reqix, AMessage *msg)
{
	DumpObject *dump = to_dump(object);

	long result = dump->io->request(dump->io, reqix, msg);
	if ((result > 0) && (dump->file != INVALID_HANDLE_VALUE)
	 && (reqix < _countof(dump->req_list)) && dump->req_list[reqix])
	{
		DWORD tx = 0;
		WriteFile(dump->file, msg->data, msg->size, &tx, NULL);
	}
	return result;
}

static long DumpCancel(AObject *object, long reqix, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	return dump->io->cancel(dump->io, reqix, msg);
}

static long DumpClose(AObject *object, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	return dump->io->close(dump->io, msg);
}

AModule DumpModule = {
	"io",
	"io_dump",
	sizeof(DumpObject),
	NULL, NULL,
	&DumpCreate,
	&DumpRelease,
	NULL,
	2,
	&DumpOpen,
	NULL,
	NULL,
	&DumpRequest,
	&DumpCancel,
	&DumpClose,
};
