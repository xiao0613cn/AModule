#include "stdafx.h"
#include "../base/AModule.h"
#include "AModule_io.h"

struct DumpObject;
struct DumpReq {
	long        reqix;
	DumpObject *dump;
	HANDLE      file;
	BOOL        attach;
	AMessage    msg;
	AMessage   *from;
	struct list_head entry;
};
struct DumpObject {
	AObject  object;
	HANDLE   file;
	char     file_name[512];
	BOOL     single_file;
	AObject *io;

	CRITICAL_SECTION req_lock;
	struct list_head req_list;
	DumpReq *req_cache[4];
};
#define to_dump(obj)   container_of(obj, DumpObject, object)

static void DumpRelease(AObject *object)
{
	DumpObject *dump = to_dump(object);

	while (!list_empty(&dump->req_list)) {
		DumpReq *req = list_first_entry(&dump->req_list, DumpReq, entry);
		list_del_init(&req->entry);

		if (!req->attach) {
			release_s(req->file, CloseHandle, INVALID_HANDLE_VALUE);
		}
		free(req);
	}
	DeleteCriticalSection(&dump->req_lock);

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
	dump->file_name[0] = '\0';
	dump->single_file = FALSE;
	dump->io = NULL;

	InitializeCriticalSection(&dump->req_lock);
	INIT_LIST_HEAD(&dump->req_list);
	memset(dump->req_cache, 0, sizeof(dump->req_cache));

	AOption *io_opt = AOptionFindChild(option, "io");
	if (io_opt != NULL)
		AObjectCreate(&dump->io, &dump->object, io_opt, NULL);

	*object = &dump->object;
	return 1;
}

static DumpReq* DumpReqGet(DumpObject *dump, long reqix)
{
	DumpReq *req;
	if (reqix < _countof(dump->req_cache)) {
		req = dump->req_cache[reqix];
		if (req != NULL)
			return req;
	}

	EnterCriticalSection(&dump->req_lock);
	list_for_each_entry(req, &dump->req_list, DumpReq, entry) {
		if (req->reqix == reqix) {
			LeaveCriticalSection(&dump->req_lock);
			return req;
		}
	}

	req = (DumpReq*)malloc(sizeof(DumpReq));
	if (req != NULL) {
		req->reqix = reqix;
		req->dump = dump;
		if (dump->single_file) {
			req->file = dump->file;
			req->attach = TRUE;
		} else {
			req->file = INVALID_HANDLE_VALUE;
			req->attach = FALSE;
		}
		req->from = NULL;

		list_add_tail(&req->entry, &dump->req_list);
		if (reqix < _countof(dump->req_cache))
			dump->req_cache[reqix] = req;
	}
	LeaveCriticalSection(&dump->req_lock);

	if (!dump->single_file && (req != NULL)) {
		char file_name[512];
		sprintf_s(file_name, "%s_%d.dmp", dump->file_name, reqix);
		req->file = CreateFileA(file_name, GENERIC_WRITE, FILE_SHARE_READ,
		                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	return req;
}

static void OnDumpRequest(DumpReq *req)
{
	AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);

	if (req->file != INVALID_HANDLE_VALUE) {
		DWORD tx = 0;
		WriteFile(req->file, req->msg.data, req->msg.size, &tx, NULL);
	} else {
		TRACE("dump(%s): reqix = %d, type = %08x, size = %d.\n",
			req->dump->file_name, req->reqix, req->msg.type, req->msg.size);
	}
}

static void OnDumpOpen(DumpObject *dump, DumpReq *req)
{
	dump->object.reqix_count = dump->io->reqix_count;
	OnDumpRequest(req);
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
	if ((opt != NULL) && (opt->value[0] != '\0') && (opt->value[1] != '\0'))
		strcpy_s(dump->file_name, opt->value);

	opt = AOptionFindChild((AOption*)msg->data, "single_file");
	if (opt != NULL)
		dump->single_file = atol(opt->value);

	if (dump->single_file && (dump->file_name[0] != '\0')) {
		release_s(dump->file, CloseHandle, INVALID_HANDLE_VALUE);

		dump->file = CreateFileA(dump->file_name, GENERIC_WRITE, FILE_SHARE_READ,
		                         NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (dump->file == INVALID_HANDLE_VALUE) {
			TRACE("dump(%s): create file error = %d.\n", dump->file_name, GetLastError());
		}
	}

	opt = AOptionFindChild((AOption*)msg->data, "io");
	if (dump->io == NULL) {
		AObjectCreate(&dump->io, &dump->object, opt, NULL);
		if (dump->io == NULL)
			return -ENXIO;
	}

	DumpReq *req = DumpReqGet(dump, 0);
	if (req == NULL)
		return -ENOMEM;

	AMsgInit(&req->msg, AMsgType_Option, (char*)opt, 0);
	req->msg.done = &DumpOpenDone;
	req->from = msg;

	long result = dump->io->open(dump->io, &req->msg);
	if (result > 0) {
		OnDumpOpen(dump, req);
	}
	return result;
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

static long DumpRequest(AObject *object, long reqix, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	DumpReq *req = DumpReqGet(dump, reqix);
	if (req == NULL)
		return dump->io->request(dump->io, reqix, msg);

	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	req->msg.done = &DumpRequestDone;
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
