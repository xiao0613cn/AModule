#include "stdafx.h"
#include "../base/AModule.h"

struct DumpObject;
struct DumpReq {
	long        reqix;
	DumpObject *dump;
	AMessage    msg;
	AMessage   *from;
};
struct DumpObject {
	AObject  object;
	HANDLE   file;
	AObject *io;
	DumpReq  req_list[4];
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
	for (int reqix = 0; reqix < _countof(dump->req_list); ++reqix) {
		dump->req_list[reqix].reqix = reqix;
		dump->req_list[reqix].dump = dump;
		dump->req_list[reqix].msg.done = NULL;
	}

	AOption *io_opt = AOptionFindChild(option, "io");
	if (io_opt != NULL)
		AObjectCreate(&dump->io, &dump->object, io_opt, NULL);

	*object = &dump->object;
	return 1;
}

static void OnDumpRequest(DumpReq *req)
{
	AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);

	DWORD tx = 0;
	WriteFile(req->dump->file, req->msg.data, req->msg.size, &tx, NULL);
}

static long DumpRequestDone(AMessage *msg, long result)
{
	DumpReq *req = container_of(msg, DumpReq, msg);
	if (result >= 0) {
		OnDumpRequest(req);
	}
	result = req->from->done(req->from, result);
	return result;
}

static void OnDumpOpen(DumpObject *dump, DumpReq *req)
{
	AOption *opt = AOptionFindChild((AOption*)req->from->data, "req_list");
	if (opt != NULL) {
		req->msg.done = NULL;
		const char *sep = opt->value;
		do {
			long reqix = atol(sep);
			if (reqix < _countof(dump->req_list)) {
				dump->req_list[reqix].msg.done = &DumpRequestDone;
			}
		} while (((sep=strchr(sep,',')) != NULL) && (*++sep != '\0'));
	} else {
		for (int reqix = 0; reqix < _countof(dump->req_list); ++reqix) {
			dump->req_list[reqix].msg.done = &DumpRequestDone;
		}
	}

	dump->object.reqix_count = dump->io->reqix_count;
}

static long DumpOpenDone(AMessage *msg, long result)
{
	DumpReq *req = container_of(msg, DumpReq, msg);
	if (result >= 0) {
		OnDumpOpen(req->dump, req);
	}
	result = req->from->done(req->from, result);
	return result;
}

static long DumpOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
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

	opt = AOptionFindChild((AOption*)msg->data, "io");
	if (dump->io == NULL) {
		AObjectCreate(&dump->io, &dump->object, opt, NULL);
		if (dump->io == NULL)
			return -ENXIO;
	}

	DumpReq *req = &dump->req_list[0];
	AMsgInit(&req->msg, AMsgType_Option, (char*)opt, 0);
	req->msg.done = &DumpOpenDone;
	req->from = msg;

	long result = dump->io->open(dump->io, &req->msg);
	if (result > 0) {
		OnDumpOpen(dump, req);
	}
	return result;
}

static long DumpRequest(AObject *object, long reqix, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	if ((dump->file == INVALID_HANDLE_VALUE)
	 || (reqix >= _countof(dump->req_list))
	 || (dump->req_list[reqix].msg.done == NULL))
		return dump->io->request(dump->io, reqix, msg);

	DumpReq *req = &dump->req_list[reqix];
	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	req->from = msg;

	long result = dump->io->request(dump->io, reqix, &req->msg);
	if (result > 0) {
		OnDumpRequest(req);
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
	if (msg != NULL) {
		release_s(dump->file, CloseHandle, INVALID_HANDLE_VALUE);
	}
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
