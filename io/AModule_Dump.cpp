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
	BOOL     trace_log;
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
			reset_s(req->file, NULL, fclose);
		}
		free(req);
	}
	pthread_mutex_destroy(&dump->req_mutex);

	reset_s(dump->file, NULL, fclose);
	release_s(dump->io);
}

static int DumpCreate(AObject **object, AObject *parent, AOption *option)
{
	DumpObject *dump = (DumpObject*)*object;
	dump->file = NULL;
	dump->io = NULL;

	pthread_mutex_init(&dump->req_mutex, NULL);
	INIT_LIST_HEAD(&dump->req_list);
	memzero(dump->req_cache);
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

	req = gomake(DumpReq);
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

	if ((req != NULL) && !dump->single_file && (dump->file_name[0] != '\0')) {
		char file_name[512];
		snprintf(file_name, sizeof(file_name), "%s_%d.dmp", dump->file_name, reqix);
		req->file = fopen(file_name, "a+b");
	}
	return req;
}

int OnDumpRequest(DumpReq *req, int result)
{
	if ((result > 0) && (req->msg.size != 0))
		req->from->init(&req->msg);

	if (req->file != NULL) {
		fwrite(req->msg.data, req->msg.size, 1, req->file);
	}
	if (req->dump->trace_log) {
		TRACE("dump(%s): reqix = %d, type = %08x, size = %d, result = %d, data:\n%.*s\n",
			req->dump->file_name, req->reqix, req->msg.type, req->msg.size, result,
			req->msg.size, req->msg.data);
	}
	return result;
}

static int Dump_init(DumpObject *dump, AOption *msg_opt)
{
	dump->trace_log = msg_opt->getInt("trace_log", TRUE);
	strcpy_sz(dump->file_name, msg_opt->getStr("file_name", ""));
	dump->single_file = msg_opt->getInt("single_file", FALSE);

	if (dump->single_file && (dump->file_name[0] != '\0')) {
		reset_s(dump->file, NULL, fclose);

		dump->file = fopen(dump->file_name, "ba+");
		if (dump->file == NULL) {
			TRACE("dump(%s): create file error = %d.\n", dump->file_name, errno);
		}
	}

	if (dump->io == NULL) {
		int result = AObject::from(&dump->io, dump, msg_opt, NULL);
		if (result < 0) {
			TRACE("require option: \"io\"\n");
			return result;
		}
	}
	return 1;
}

static int DumpOpen(AObject *object, AMessage *msg)
{
	if ((msg->type != AMsgType_AOption)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	DumpObject *dump = (DumpObject*)object;
	AOption *msg_opt = (AOption*)msg->data;
	int result = Dump_init(dump, msg_opt);
	if (result < 0)
		return result;

	DumpReq *req = DumpReqGet(dump, 0);
	if (req == NULL)
		return -ENOMEM;

	req->msg.init(msg_opt->find("io"));
	req->msg.done = &MsgProxyC(DumpReq, msg, from, OnDumpRequest);
	req->from = msg;

	result = dump->io->open(&req->msg);
	if (result != 0)
		result = OnDumpRequest(req, result);
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
		return dump->io->request(reqix, msg);

	req->msg.init(msg);
	req->msg.done = &MsgProxyC(DumpReq, msg, from, OnDumpRequest);
	req->from = msg;

	int result = dump->io->request(reqix, &req->msg);
	if (result != 0)
		result = OnDumpRequest(req, result);
	return result;
}

static int DumpCancel(AObject *object, int reqix, AMessage *msg)
{
	DumpObject *dump = (DumpObject*)object;
	if (dump->io == NULL)
		return -ENOENT;
	return dump->io->cancel(reqix, msg);
}

static int DumpClose(AObject *object, AMessage *msg)
{
	DumpObject *dump = (DumpObject*)object;
	if (dump->io == NULL)
		return -ENOENT;
	return dump->io->close(msg);
}

static int DumpSvcAccept(AObject *object, AMessage *msg, AObject *svc_data, AOption *svc_opt)
{
	DumpObject *dump = (DumpObject*)object;
	int result = Dump_init(dump, svc_opt);
	if (result < 0)
		return result;

	DumpReq *req = DumpReqGet(dump, 0);
	if (req == NULL)
		return -ENOMEM;

	req->msg.init(msg);
	req->msg.done = &MsgProxyC(DumpReq, msg, from, OnDumpRequest);
	req->from = msg;

	result = dump->io->M()->svc_accept(dump->io, &req->msg, svc_data, svc_opt->find("io"));
	if (result != 0)
		result = OnDumpRequest(req, result);
	return result;
}

static int DumpSvcCreate(AObject **svc_data, AObject *parent, AOption *option)
{
	AOption *io_opt = option->find("io");
	if (io_opt == NULL)
		return -EINVAL;

	IOModule *io = (IOModule*)AModuleFind("io", io_opt->value);
	if ((io == NULL) || (io->svc_accept == NULL))
		return -EINVAL;

	if (io->svc_module == NULL)
		return 1;
	return AObject::create2(svc_data, parent, io_opt, io->svc_module);
}

AModule DumpSvcModule = {
	"DumpSvcModule",
	"DumpSvcModule",
	0, NULL, NULL,
	&DumpSvcCreate,
};
static int reg_svc = AModuleRegister(&DumpSvcModule);

IOModule DumpModule = { {
	"io",
	"io_dump",
	sizeof(DumpObject),
	NULL, NULL,
	&DumpCreate,
	&DumpRelease, },
	&DumpOpen,
	&DumpSetOption,
	&DumpGetOption,
	&DumpRequest,
	&DumpCancel,
	&DumpClose,

	&DumpSvcModule,
	&DumpSvcAccept,
};
static int reg_dumpio = AModuleRegister(&DumpModule.module);
