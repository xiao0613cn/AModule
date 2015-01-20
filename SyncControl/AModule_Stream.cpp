#include "stdafx.h"
#include "../base/AModule.h"


struct SyncRequest {
	long      reqix;
	AMessage  msg;
	AMessage *from;
	long      msgloop : 1;
	long      abortloop : 1;
	struct SyncControl *sc;
	struct list_head entry;

	CRITICAL_SECTION lock;
	struct list_head request_list;
	struct list_head notify_list;
};
#define to_req(msg) container_of(msg, SyncRequest, msg)

enum StreamStatus {
	stream_invalid = 0,
	stream_opening,
	stream_opened,
	stream_abort,
	stream_closing,
	stream_closed,
};

struct SyncControl {
	AObject   object;
	AObject  *stream;
	long      open_result;
	AMessage *close_msg;

	long volatile status;
	long volatile request_count;
	struct list_head syncreq_list;
};
#define to_sc(obj) container_of(obj, SyncControl, object)

static SyncRequest* SyncRequestGet(SyncControl *sc, long reqix)
{
	SyncRequest *req;
	list_for_each_entry(req, &sc->syncreq_list, SyncRequest, entry) {
		if (req->reqix == reqix)
			return req;
	}
	return NULL;
}

static long SyncRequestDone(AMessage *msg, long result);

static SyncRequest* SyncRequestNew(SyncControl *sc, long reqix)
{
	SyncRequest *req = (SyncRequest*)malloc(sizeof(SyncRequest));
	if (req == NULL)
		return NULL;

	req->reqix = reqix;
	req->msg.done = &SyncRequestDone;
	req->from = NULL;
	req->msgloop = 0;
	req->abortloop = 0;
	req->sc = sc;
	list_add(&req->entry, &sc->syncreq_list);

	InitializeCriticalSection(&req->lock);
	INIT_LIST_HEAD(&req->request_list);
	INIT_LIST_HEAD(&req->notify_list);
	return req;
}

static void SyncControlRelease(AObject *object)
{
	SyncControl *sc = to_sc(object);
	release_s(sc->stream, AObjectRelease, NULL);

	while (!list_empty(&sc->syncreq_list)) {
		SyncRequest *req = list_first_entry(&sc->syncreq_list, SyncRequest, entry);
		list_del_init(&req->entry);

		if (req->from != NULL) {
			req->from->done(req->from, -EINTR);
			req->from = NULL;
		}
		DeleteCriticalSection(&req->lock);
		MsgListClear(&req->request_list, -EINTR);
		MsgListClear(&req->notify_list, -EINTR);
		free(req);
	}
	free(sc);
}

static long SyncControlCreate(AObject **object, AObject *parent, AOption *option)
{
	SyncControl *sc = (SyncControl*)malloc(sizeof(SyncControl));
	if (sc == NULL)
		return -ENOMEM;

	extern AModule SyncControlModule;
	AObjectInit(&sc->object, &SyncControlModule);
	sc->stream = NULL;
	sc->status = stream_invalid;
	sc->open_result = 0;
	sc->close_msg = NULL;

	INIT_LIST_HEAD(&sc->syncreq_list);
	sc->request_count = 0;
	*object = &sc->object;

	long result = AObjectCreate(&sc->stream, parent, option, NULL);
	return result;
}

static long inline SyncControlChangeStatus(SyncControl *sc, SyncRequest *req,
                                           enum StreamStatus new_status, enum StreamStatus test_status)
{
	AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);
	req->msg.done = &SyncRequestDone;
	req->from = NULL;

	return InterlockedCompareExchange(&sc->status, new_status, test_status);
}

static long SyncControlOpenStatus(SyncRequest *req, long result)
{
	SyncControl *sc = req->sc;
	long old_status;
	switch (sc->status)
	{
	case stream_abort:
		result = -EINTR;
	case stream_opening:
		if (result == 0)
			result = 1;
		sc->open_result = result;
		if (result > 0)
		{
			sc->object.reqix_count = sc->stream->reqix_count;
			for (int ix=0; ix<sc->object.reqix_count; ++ix)
			{
				SyncRequest *req2 = SyncRequestGet(sc, ix);
				if (req2 != NULL)
					continue;
				req2 = SyncRequestNew(sc, ix);
				if (req2 != NULL)
					continue;
				result = -ENOMEM;
				break;
			}
		}
		if (result > 0) {
			old_status = SyncControlChangeStatus(sc, req, stream_opened, stream_opening);
			if (old_status == stream_opening)
				return result;
		}

		old_status = InterlockedExchange(&sc->status, stream_closing);
		assert((old_status == stream_opening) || (old_status == stream_abort));
		result = sc->stream->close(sc->stream, &req->msg);
		if (result == 0)
			return 0;

	case stream_closing:
		old_status = InterlockedDecrement(&sc->request_count);
		assert(old_status == 0);
		result = sc->open_result;

		old_status = SyncControlChangeStatus(sc, req, stream_closed, stream_closing);
		assert(old_status == stream_closing);
		return result;

	default:
		assert(FALSE);
		return -EACCES;
	}
}

static long SyncControlOpenDone(AMessage *msg, long result)
{
	SyncRequest *req = to_req(msg);
	msg = req->from;

	result = SyncControlOpenStatus(req, result);
	if (result != 0)
		result = msg->done(msg, result);
	return result;
}

static long SyncControlOpen(AObject *object, AMessage *msg)
{
	SyncControl *sc = to_sc(object);

	long result = InterlockedCompareExchange(&sc->status, stream_opening, stream_invalid);
	if (result != stream_invalid)
		return -EBUSY;

	SyncRequest *req = SyncRequestGet(sc, 0);
	if (req == NULL) {
		req = SyncRequestNew(sc, 0);
		if (req == NULL)
			return -ENOMEM;
	}

	result = InterlockedIncrement(&sc->request_count);
	assert(result == 1);

	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	req->msg.done = &SyncControlOpenDone;
	req->from = msg;

	result = sc->stream->open(sc->stream, &req->msg);
	if (result != 0)
		result = SyncControlOpenStatus(req, result);
	return result;
}

static long SyncControlSetOption(AObject *object, AOption *option)
{
	SyncControl *sc = to_sc(object);
	long result = -ENOSYS;
	if (sc->stream->setopt != NULL)
		result = sc->stream->setopt(sc->stream, option);
	return result;
}

static long SyncControlGetOption(AObject *object, AOption *option)
{
	SyncControl *sc = to_sc(object);
	long result = -ENOSYS;
	if (sc->stream->getopt != NULL)
		result = sc->stream->getopt(sc->stream, option);
	return result;
}

static long SyncControlCloseDone(AMessage *msg, long result)
{
	SyncRequest *req = to_req(msg);
	SyncControl *sc = req->sc;
	msg = sc->close_msg;

	assert(sc->close_msg == req->from);
	sc->close_msg = NULL;
	long old_status = SyncControlChangeStatus(sc, req, stream_closed, stream_closing);
	assert(old_status == stream_closing);

	result = msg->done(msg, result);
	return result;
}

static long SyncControlDoClose(SyncControl *sc, SyncRequest *req)
{
	AMsgInit(&req->msg, sc->close_msg->type, sc->close_msg->data, sc->close_msg->size);
	req->msg.done = &SyncControlCloseDone;
	req->from = sc->close_msg;

	return sc->stream->close(sc->stream, &req->msg);
}

static void SyncRequestDispatch(SyncRequest *req, long result)
{
	SyncControl *sc = req->sc;
	AMessage *msg;
	for (;;) {
		EnterCriticalSection(&req->lock);
		list_for_each_entry(msg, &req->notify_list, AMessage, entry) {
			if ((msg->type == AMsgType_Unknown) || (msg->type == req->msg.type))
			{
				AMsgInit(msg, req->msg.type, req->msg.data, req->msg.size);
				if (msg->done(msg, result) > 0)
					break;
			}
		}
		if (req->msgloop) {
			if (result < 0) {
				msg = NULL;
			} else if (req->abortloop || (sc->status != stream_opened)) {
				msg = req->from;
				result = -EINTR;
			} else {
				msg = req->from;
				result = 1;
			}
			if (result < 0) {
				req->from = NULL;
				req->msgloop = 0;
				req->abortloop = 0;
			}
		} else if (list_empty(&req->request_list)) {
			msg = req->from = NULL;
			result = 0;
		} else {
			msg = req->from = list_first_entry(&req->request_list, AMessage, entry);
			list_del_init(&msg->entry);
			result = 1;
		}
		LeaveCriticalSection(&req->lock);
		if (result <= 0)
			break;

		AMsgInit(&req->msg, msg->type, msg->data, msg->size);
		result = sc->stream->request(sc->stream, req->reqix, &req->msg);
		if (result == 0)
			return;

		AMsgInit(msg, req->msg.type, req->msg.data, req->msg.size);
		msg->done(msg, result);
	}

	if (msg != NULL)
		msg->done(msg, result);

	BOOL do_close = (InterlockedDecrement(&sc->request_count) == 0);
	if (do_close) {
		MsgListClear(&req->request_list, result);

		result = SyncControlDoClose(sc, req);
		if (result != 0) {
			msg = sc->close_msg;

			assert(sc->close_msg == req->from);
			sc->close_msg = NULL;
			long old_status = SyncControlChangeStatus(sc, req, stream_closed, stream_closing);
			assert(old_status == stream_closing);

			msg->done(msg, result);
		}
	}
	AObjectRelease(&sc->object);
}

static long SyncRequestDone(AMessage *msg, long result)
{
	SyncRequest *req = to_req(msg);
	msg = req->from;

	AMsgInit(msg, req->msg.type, req->msg.data, req->msg.size);
	msg->done(msg, result);

	SyncRequestDispatch(req, result);
	return result;
}

static long SyncControlRequest(AObject *object, long reqix, AMessage *msg)
{
	SyncControl *sc = to_sc(object);
	if (sc->status != stream_opened)
		return -ENOENT;

	long flag = (reqix & ~ARequest_IndexMask);
	reqix = reqix & ARequest_IndexMask;
	SyncRequest *req = SyncRequestGet(sc, reqix);
	if (req == NULL)
		return -ENOENT;

	long result;
	EnterCriticalSection(&req->lock);
	if (sc->status != stream_opened) {
		result = -EINTR;
	} else switch (flag)
	{
	case ANotify_InQueue:
		list_add_tail(&msg->entry, &req->notify_list);
		result = 0;
		break;
	case ARequest_MsgLoop:
		if (req->from != NULL) {
			result = -EBUSY;
			break;
		}
		req->msgloop = 1;
		req->abortloop = 0;
	default:
		if (req->from == NULL) {
			req->from = msg;
			InterlockedIncrement(&sc->request_count);
			result = 1;
		} else {
			list_add_tail(&msg->entry, &req->request_list);
			result = 0;
		}
		break;
	}
	LeaveCriticalSection(&req->lock);
	if (result <= 0)
		return result;

	AObjectAddRef(&sc->object);
	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	result = sc->stream->request(sc->stream, reqix, &req->msg);
	if (result != 0) {
		SyncRequestDispatch(req, result);
	}
	return result;
}

static long SyncControlCancel(AObject *object, long reqix, AMessage *msg)
{
	SyncControl *sc = to_sc(object);
	if (sc->status != stream_opened)
		return -ENOENT;

	long flag = (reqix & ~ARequest_IndexMask);
	reqix = reqix & ARequest_IndexMask;
	SyncRequest *req = SyncRequestGet(sc, reqix);
	if (req == NULL)
		return -ENOENT;

	struct list_head *head;
	if (flag == ANotify_InQueue) {
		head = &req->notify_list;
	} else {
		head = &req->request_list;
	}

	AMessage *pos;
	long result = -ENOENT;
	EnterCriticalSection(&req->lock);
	if (msg == req->from) {
		if (req->msgloop) {
			req->abortloop = 1;
			result = 0;
		} else {
			result = -EBUSY;
		}
	} else {
		list_for_each_entry(pos, head, AMessage, entry) {
			if (pos == msg) {
				list_del_init(&msg->entry);
				result = 1;
				break;
			}
		}
	}
	LeaveCriticalSection(&req->lock);
	return result;
}

static long SyncControlClose(AObject *object, AMessage *msg)
{
	SyncControl *sc = to_sc(object);
	if (msg == NULL)
		return sc->stream->close(sc->stream, NULL);

	long new_status = stream_abort;
	long test_status = stream_opening;
	for (;;) {
		long old_status = InterlockedCompareExchange(&sc->status, new_status, test_status);
		if ((old_status == stream_invalid) || (old_status == stream_closed))
			return -ENOENT;
		if ((old_status == stream_abort) || (old_status == stream_closing))
			return -EBUSY;
		if (old_status != test_status) {
			test_status = stream_opened;
			new_status = stream_closing;
			Sleep(0);
			continue;
		}
		if (old_status == stream_opening)
			return 1;
		break;
	}

	SyncRequest *req;
	list_for_each_entry(req, &sc->syncreq_list, SyncRequest, entry) {
		EnterCriticalSection(&req->lock);
		LeaveCriticalSection(&req->lock);
	}

	sc->close_msg = msg;
	if (InterlockedDecrement(&sc->request_count) != 0)
		return 0;

	req = SyncRequestGet(sc, 0);
	long result = SyncControlDoClose(sc, req);
	if (result != 0) {
		assert(sc->close_msg == req->from);
		sc->close_msg = NULL;
		long old_status = SyncControlChangeStatus(sc, req, stream_closed, stream_closing);
		assert(old_status == stream_closing);
	}
	return result;
}

AModule SyncControlModule = {
	"stream",
	"SyncControl",
	sizeof(SyncControl),
	NULL, NULL,
	&SyncControlCreate,
	&SyncControlRelease,
	0,

	&SyncControlOpen,
	&SyncControlSetOption,
	&SyncControlGetOption,
	&SyncControlRequest,
	&SyncControlCancel,
	&SyncControlClose,
};
