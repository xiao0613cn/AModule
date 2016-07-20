#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"

struct DumpObject;
struct DumpReq {
	int         reqix;
	DumpObject *dump;
	FILE       *file;
	BOOL        attach;
	AMessage    msg;
	AMessage   *from;
	struct list_head entry;
};
struct DumpObject {
	AObject  object;
	FILE    *file;
	char     file_name[512];
	BOOL     single_file;
	AObject *io;

	pthread_mutex_t  req_mutex;
	struct list_head req_list;
	DumpReq         *req_cache[4];
};
#define to_dump(obj)   container_of(obj, DumpObject, object)

static void DumpRelease(AObject *object)
{
	DumpObject *dump = to_dump(object);

	while (!list_empty(&dump->req_list)) {
		DumpReq *req = list_first_entry(&dump->req_list, DumpReq, entry);
		list_del_init(&req->entry);

		if (!req->attach) {
			release_s(req->file, fclose, NULL);
		}
		free(req);
	}
	pthread_mutex_destroy(&dump->req_mutex);

	release_s(dump->file, fclose, NULL);
	release_s(dump->io, AObjectRelease, NULL);

	free(dump);
}

static int DumpCreate(AObject **object, AObject *parent, AOption *option)
{
	DumpObject *dump = (DumpObject*)malloc(sizeof(DumpObject));
	if (dump == NULL)
		return -ENOMEM;

	extern AModule DumpModule;
	AObjectInit(&dump->object, &DumpModule);
	dump->file = NULL;
	dump->file_name[0] = '\0';
	dump->single_file = FALSE;
	dump->io = NULL;

	pthread_mutex_init(&dump->req_mutex, NULL);
	INIT_LIST_HEAD(&dump->req_list);
	memset(dump->req_cache, 0, sizeof(dump->req_cache));

	AOption *io_opt = AOptionFind(option, "io");
	if (io_opt != NULL)
		AObjectCreate(&dump->io, &dump->object, io_opt, NULL);

	*object = &dump->object;
	return 1;
}

static DumpReq* DumpReqGet(DumpObject *dump, int reqix)
{
	DumpReq *req;
	if (reqix < _countof(dump->req_cache)) {
		req = dump->req_cache[reqix];
		if (req != NULL)
			return req;
	}

	pthread_mutex_lock(&dump->req_mutex);
	list_for_each_entry(req, &dump->req_list, DumpReq, entry) {
		if (req->reqix == reqix) {
			pthread_mutex_unlock(&dump->req_mutex);
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
			req->file = NULL;
			req->attach = FALSE;
		}
		req->from = NULL;

		list_add_tail(&req->entry, &dump->req_list);
		if (reqix < _countof(dump->req_cache))
			dump->req_cache[reqix] = req;
	}
	pthread_mutex_unlock(&dump->req_mutex);

	if (!dump->single_file && (req != NULL)) {
		char file_name[512];
		sprintf_s(file_name, "%s_%d.dmp", dump->file_name, reqix);
		req->file = fopen(file_name, "ba+");
	}
	return req;
}

static void OnDumpRequest(DumpReq *req)
{
	AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);

	if (req->file != NULL) {
		fwrite(req->msg.data, req->msg.size, 1, req->file);
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

static int DumpOpenDone(AMessage *msg, int result)
{
	DumpReq *req = container_of(msg, DumpReq, msg);
	if (result >= 0) {
		OnDumpOpen(req->dump, req);
	}
	result = req->from->done(req->from, result);
	return result;
}

static int DumpOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	DumpObject *dump = to_dump(object);
	AOption *opt = AOptionFind((AOption*)msg->data, "file");
	if ((opt != NULL) && (opt->value[0] != '\0') && (opt->value[1] != '\0'))
		strcpy_s(dump->file_name, opt->value);

	opt = AOptionFind((AOption*)msg->data, "single_file");
	if (opt != NULL)
		dump->single_file = atol(opt->value);

	if (dump->single_file && (dump->file_name[0] != '\0')) {
		release_s(dump->file, fclose, NULL);

		dump->file = fopen(dump->file_name, "ba+");
		if (dump->file == NULL) {
			TRACE("dump(%s): create file error = %d.\n", dump->file_name, errno);
		}
	}

	opt = AOptionFind((AOption*)msg->data, "io");
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

	int result = dump->io->open(dump->io, &req->msg);
	if (result > 0) {
		OnDumpOpen(dump, req);
	}
	return result;
}

static int DumpSetOption(AObject *object, AOption *option)
{
	DumpObject *dump = to_dump(object);
	if (dump->io == NULL)
		return -ENOENT;

	if (dump->io->setopt == NULL)
		return -ENOSYS;

	return dump->io->setopt(dump->io, option);
}

static int DumpGetOption(AObject *object, AOption *option)
{
	DumpObject *dump = to_dump(object);
	if (dump->io == NULL)
		return -ENOENT;

	if (dump->io->getopt == NULL)
		return -ENOSYS;

	return dump->io->getopt(dump->io, option);
}

static int DumpRequestDone(AMessage *msg, int result)
{
	DumpReq *req = container_of(msg, DumpReq, msg);
	if (result >= 0) {
		OnDumpRequest(req);
	}
	result = req->from->done(req->from, result);
	return result;
}

static int DumpRequest(AObject *object, int reqix, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	DumpReq *req = DumpReqGet(dump, reqix);
	if (req == NULL)
		return dump->io->request(dump->io, reqix, msg);

	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	req->msg.done = &DumpRequestDone;
	req->from = msg;

	int result = dump->io->request(dump->io, reqix, &req->msg);
	if (result > 0) {
		OnDumpRequest(req);
	}
	return result;
}

static int DumpCancel(AObject *object, int reqix, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	return dump->io->cancel(dump->io, reqix, msg);
}

static int DumpClose(AObject *object, AMessage *msg)
{
	DumpObject *dump = to_dump(object);
	if (msg != NULL) {
		release_s(dump->file, fclose, NULL);
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
	&DumpSetOption,
	&DumpGetOption,
	&DumpRequest,
	&DumpCancel,
	&DumpClose,
};
