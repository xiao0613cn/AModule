#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"


struct DumpReq {
	int         reqix;
	struct DumpObject *dump;
	FILE       *file;
	AMessage    msg;
	AMessage   *from;
	struct list_head entry;
};

struct DumpObject : public IOObject {
	FILE    *file;
	char     file_name[BUFSIZ];
	BOOL     single_file;
	IOObject *io;

	pthread_mutex_t  req_mutex;
	struct list_head req_list;
	DumpReq         *req_cache[4];
};

static void DumpRelease(AObject *object)
{
	DumpObject *dump = (DumpObject*)object;

	while (!list_empty(&dump->req_list)) {
		DumpReq *req = list_pop_front(&dump->req_list, DumpReq, entry);
		if (!dump->single_file) {
			release_s(req->file, fclose, NULL);
		}
		free(req);
	}
	pthread_mutex_destroy(&dump->req_mutex);

	release_s(dump->file, fclose, NULL);
	release_s(dump->io, AObjectRelease, NULL);
}

static int DumpCreate(AObject **object, AObject *parent, AOption *option)
{
	DumpObject *dump = (DumpObject*)*object;
	dump->file = NULL;

	pthread_mutex_init(&dump->req_mutex, NULL);
	INIT_LIST_HEAD(&dump->req_list);
	memset(dump->req_cache, 0, sizeof(dump->req_cache));
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
		} else {
			req->file = NULL;
		}
		req->from = NULL;

		list_add_tail(&req->entry, &dump->req_list);
		if (reqix < _countof(dump->req_cache))
			dump->req_cache[reqix] = req;
	}
	pthread_mutex_unlock(&dump->req_mutex);

	if (!dump->single_file && (req != NULL)) {
		char file_name[512];
		snprintf(file_name, sizeof(file_name), "%s_%d.dmp", dump->file_name, reqix);
		req->file = fopen(file_name, "a+b");
	}
	return req;
}

int OnDumpRequest(DumpReq *req, int result)
{
	if (result < 0) {
		return result;
	}

	DumpObject *dump = req->dump;
	//if (dump->object.reqix_count == 0) // open done
	//	dump->object.reqix_count = dump->io->reqix_count;

	req->from->init(req->msg);
	if (req->file != NULL) {
		fwrite(req->msg.data, req->msg.size, 1, req->file);
	} else {
		TRACE("dump(%s): reqix = %d, type = %08x, size = %d, result = %d.\n",
			dump->file_name, req->reqix, req->msg.type, req->msg.size, result);
	}
	return result;
}

static int DumpOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *msg_opt = (AOption*)msg->data;
	DumpObject *dump = (DumpObject*)object;

	strcpy_sz(dump->file_name, msg_opt->getStr("file_name", "io_dump"));
	dump->single_file = msg_opt->getInt("single_file", FALSE);

	if (dump->single_file && (dump->file_name[0] != '\0')) {
		release_s(dump->file, fclose, NULL);

		dump->file = fopen(dump->file_name, "ba+");
		if (dump->file == NULL) {
			TRACE("dump(%s): create file error = %d.\n", dump->file_name, errno);
		}
	}

	AOption *io_opt = AOptionFind(msg_opt, "io");
	if (dump->io == NULL) {
		int result = AObjectCreate((AObject**)&dump->io, dump, io_opt, NULL);
		if (dump->io == NULL)
			return result;
	}

	DumpReq *req = DumpReqGet(dump, 0);
	if (req == NULL)
		return -ENOMEM;

	req->msg.init(io_opt);
	req->msg.done = &TObjectDone(DumpReq, msg, from, OnDumpRequest);
	req->from = msg;

	int result = dump->io->open(&req->msg);
	if (result != 0) {
		OnDumpRequest(req, result);
	}
	return result;
}

static int DumpSetOption(AObject *object, AOption *option)
{
	DumpObject *dump = (DumpObject*)object;
	if (dump->io == NULL)
		return -ENOENT;
	return dump->io->setopt(option);
}

static int DumpGetOption(AObject *object, AOption *option)
{
	DumpObject *dump = (DumpObject*)object;
	if (dump->io == NULL)
		return -ENOENT;
	return dump->io->getopt(option);
}

static int DumpRequest(AObject *object, int reqix, AMessage *msg)
{
	DumpObject *dump = (DumpObject*)object;
	DumpReq *req = DumpReqGet(dump, reqix);
	if (req == NULL)
		return (*dump->io)->request(dump->io, reqix, msg);

	req->msg.init(msg);
	req->msg.done = &TObjectDone(DumpReq, msg, from, OnDumpRequest);
	req->from = msg;

	int result = (*dump->io)->request(dump->io, reqix, &req->msg);
	if (result != 0) {
		OnDumpRequest(req, result);
	}
	return result;
}

static int DumpCancel(AObject *object, int reqix, AMessage *msg)
{
	DumpObject *dump = (DumpObject*)object;
	if (dump->io == NULL)
		return -ENOENT;
	return (*dump->io)->cancel(dump->io, reqix, msg);
}

static int DumpClose(AObject *object, AMessage *msg)
{
	DumpObject *dump = (DumpObject*)object;
	if (msg != NULL) {
		release_s(dump->file, fclose, NULL);
	}
	return dump->io->close(msg);
}

IOModule DumpModule = { {
	"io",
	"io_dump",
	sizeof(DumpObject),
	NULL, NULL,
	&DumpCreate,
	&DumpRelease,
	NULL, },
	&DumpOpen,
	&DumpSetOption,
	&DumpGetOption,
	&DumpRequest,
	&DumpCancel,
	&DumpClose,
};

static auto_reg_t reg(DumpModule.module);
