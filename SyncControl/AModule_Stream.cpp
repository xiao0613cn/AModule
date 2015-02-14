#include "stdafx.h"
#include "../base/AModule.h"


struct SyncRequest {
	struct SyncControl *sc;
	long                reqix;
	struct list_head    entry;
	AMessage            msg;

	CRITICAL_SECTION    lock;
	AMessage           *from;
	unsigned long       msgloop : 1;
	unsigned long       abortloop : 1;
	struct list_head    request_list;
	struct list_head    notify_list;
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

#define syncreq_cache_count  32

struct SyncControl {
	AObject   object;
	AObject  *stream;
	long      open_result;
	AMessage *close_msg;

	StreamStatus volatile status;
	long volatile request_count;
	struct list_head syncreq_list;
	SyncRequest     *syncreq_cache_list[syncreq_cache_count];
};
#define to_sc(obj) container_of(obj, SyncControl, object)

static SyncRequest* SyncRequestGet(SyncControl *sc, long reqix)
{
	if (reqix < syncreq_cache_count)
		return sc->syncreq_cache_list[reqix];

	if (reqix > sc->object.reqix_count)
		return NULL;

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

	req->sc = sc;
	req->reqix = reqix;
	list_add(&req->entry, &sc->syncreq_list);
	req->msg.done = &SyncRequestDone;
	INIT_LIST_HEAD(&req->msg.entry);

	InitializeCriticalSection(&req->lock);
	req->from = NULL;
	req->msgloop = 0;
	req->abortloop = 0;
	INIT_LIST_HEAD(&req->request_list);
	INIT_LIST_HEAD(&req->notify_list);

	if (reqix < syncreq_cache_count)
		sc->syncreq_cache_list[reqix] = req;
	return req;
}

static void SyncControlRelease(AObject *object)
{
	SyncControl *sc = to_sc(object);
	release_s(sc->stream, AObjectRelease, NULL);
	if (sc->close_msg != NULL) {
		sc->close_msg->done(sc->close_msg, -EINTR);
		sc->close_msg = NULL;
	}
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
	sc->open_result = 0;
	sc->close_msg = NULL;
	sc->status = stream_invalid;
	sc->request_count = 0;
	INIT_LIST_HEAD(&sc->syncreq_list);
	memset(sc->syncreq_cache_list, 0, sizeof(sc->syncreq_cache_list));

	*object = &sc->object;
	long result = AObjectCreate(&sc->stream, parent, option, NULL);
	return result;
}

static long SyncControlOpenDone(AMessage *msg, long result);

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
		if (result > 0)
		{
			while (sc->object.reqix_count < sc->stream->reqix_count) {
				if (SyncRequestNew(sc, sc->object.reqix_count) == NULL) {
					result = -ENOMEM;
					break;
				}
				++sc->object.reqix_count;
			}
		}
		AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);
		sc->open_result = result;
		if (result > 0) {
			AMessage *msg = req->from;
			req->msg.done = &SyncRequestDone;
			req->from = NULL;
			old_status = InterlockedCompareExchange((long volatile*)&sc->status, stream_opened, stream_opening);
			if (old_status == stream_opening)
				break;

			sc->open_result = -EINTR;
			req->msg.done = &SyncControlOpenDone;
			req->from = msg;
		}

		old_status = InterlockedExchange((long volatile*)&sc->status, stream_closing);
		assert((old_status == stream_opening) || (old_status == stream_abort));

		AMsgInit(&req->msg, AMsgType_Unknown, NULL, 0);
		result = sc->stream->close(sc->stream, &req->msg);
		if (result == 0)
			break;

	case stream_closing:
		result = InterlockedDecrement(&sc->request_count);
		assert(result == 0);
		result = sc->open_result;

		req->msg.done = &SyncRequestDone;
		req->from = NULL;
		old_status = InterlockedExchange((long volatile*)&sc->status, stream_closed);
		assert(old_status == stream_closing);
		break;

	default:
		assert(FALSE);
		result = -EACCES;
		break;
	}
	return result;
}

static long SyncControlOpenDone(AMessage *msg, long result)
{
	SyncRequest *req = to_req(msg);
	msg = req->from;

	result = SyncControlOpenStatus(req, result);
	if (result != 0) {
		if (msg->done != NULL) {
			result = msg->done(msg, result);
		} else {
			msg->entry.prev = (struct list_head*)result;
			msg->entry.next = LIST_POISON1;
		}
	}
	return result;
}

static long SyncControlOpen(AObject *object, AMessage *msg)
{
	SyncControl *sc = to_sc(object);

	long result = InterlockedCompareExchange((long volatile*)&sc->status, stream_opening, stream_invalid);
	if (result != stream_invalid)
		return -EBUSY;

	SyncRequest *req = SyncRequestGet(sc, 0);
	if (req == NULL) {
		req = SyncRequestNew(sc, 0);
		if (req == NULL)
			return -ENOMEM;
		sc->object.reqix_count = 1;
	}

	result = InterlockedIncrement(&sc->request_count);
	assert(result == 1);

	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	req->msg.done = &SyncControlOpenDone;
	req->from = msg;
	if (msg->done == NULL)
		msg->entry.next = NULL;

	result = sc->stream->open(sc->stream, &req->msg);
	if (result != 0)
		result = SyncControlOpenStatus(req, result);
	if ((result == 0) && (msg->done == NULL)) {
		while (msg->entry.next == NULL)
			Sleep(10);
		result = (long)msg->entry.prev;
	}
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

static void SyncControlClosed(SyncControl *sc, SyncRequest *req)
{
	assert(sc->close_msg == req->from);
	sc->close_msg = NULL;

	AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);
	req->msg.done = &SyncRequestDone;
	req->from = NULL;

	long old_status = InterlockedExchange((long volatile*)&sc->status, stream_closed);
	assert(old_status == stream_closing);
}

static long SyncControlCloseDone(AMessage *msg, long result)
{
	SyncRequest *req = to_req(msg);
	msg = req->from;
	SyncControlClosed(req->sc, req);
	result = msg->done(msg, result);
	return result;
}

static long SyncControlDoClose(SyncControl *sc, SyncRequest *req)
{
	SyncRequest *pos;
	list_for_each_entry(pos, &sc->syncreq_list, SyncRequest, entry) {
		MsgListClear(&pos->request_list, -EINTR);
		MsgListClear(&pos->notify_list, -EINTR);
	}

	AMsgInit(&req->msg, sc->close_msg->type, sc->close_msg->data, sc->close_msg->size);
	req->msg.done = &SyncControlCloseDone;
	req->from = sc->close_msg;

	return sc->stream->close(sc->stream, &req->msg);
}

static void SyncRequestDispatch(SyncRequest *req, long result, BOOL firstloop)
{
	SyncControl *sc = req->sc;
	AMessage *msg;
	for (;;) {
		AMessage *probe_msg = NULL;
		long      probe_ret;

		EnterCriticalSection(&req->lock);
		if (result >= 0) {
		list_for_each_entry(msg, &req->notify_list, AMessage, entry) {
			if ((msg->type == AMsgType_Unknown) || (msg->type == req->msg.type))
			{
				AMsgInit(msg, req->msg.type, req->msg.data, req->msg.size);
				probe_ret = msg->done(msg, 0);
				if (probe_ret == 0)
					continue;

				probe_msg = msg;
				if (probe_ret > 0) {
					list_del_init(&probe_msg->entry);
					break;
				}

				msg = list_entry(msg->entry.prev, AMessage, entry);
				list_move(&probe_msg->entry, &req->msg.entry);
				probe_msg = NULL;
			}
		} }
		if (req->msgloop) {
			msg = req->from;
			if (result < 0) {
				if (firstloop)
					msg = NULL;
			} else if (req->abortloop || (sc->status != stream_opened)) {
				result = -EINTR;
			} else {
				result = 1;
			}
			if (result < 0) {
				req->from = NULL;
				req->msgloop = 0;
				req->abortloop = 0;
			}
		} else if (sc->status != stream_opened) {
			msg = req->from = NULL;
			result = -EINTR;
		} else if (list_empty(&req->request_list)) {
			msg = req->from = NULL;
			result = 0;
		} else {
			msg = req->from = list_first_entry(&req->request_list, AMessage, entry);
			list_del_init(&msg->entry);
			result = 1;
		}
		LeaveCriticalSection(&req->lock);

		if (probe_msg != NULL) {
			probe_msg->done(probe_msg, probe_ret);
		}
		while (!list_empty(&req->msg.entry)) {
			probe_msg = list_first_entry(&req->msg.entry, AMessage, entry);
			list_del_init(&probe_msg->entry);
			probe_msg->done(probe_msg, -1);
		}
		if (result <= 0)
			break;

		AMsgInit(&req->msg, msg->type, msg->data, msg->size);
		result = sc->stream->request(sc->stream, req->reqix, &req->msg);
		if (result == 0)
			return;

		if (!req->msgloop || (result >= 0)) {
			AMsgInit(msg, req->msg.type, req->msg.data, req->msg.size);
			msg->done(msg, result);
		}
		firstloop = FALSE;
	}

	if (msg != NULL)
		msg->done(msg, result);

	if (InterlockedDecrement(&sc->request_count) == 0) {
		result = SyncControlDoClose(sc, req);
		if (result != 0) {
			msg = sc->close_msg;
			SyncControlClosed(sc, req);
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
	if (!req->msgloop || (result >= 0)) {
		msg->done(msg, result);
	}

	SyncRequestDispatch(req, result, FALSE);
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
	} else {
		switch (flag)
		{
		case ANotify_InQueueFront:
			list_add(&msg->entry, &req->notify_list);
			result = 0;
			break;
		case ANotify_InQueueBack:
			list_add_tail(&msg->entry, &req->notify_list);
			result = 0;
			break;
		case ARequest_MsgLoop:
			if (req->msgloop || (req->from != NULL)) {
				result = -EBUSY;
			} else {
				req->msgloop = 1;
				req->abortloop = 0;
				req->from = msg;
				result = InterlockedIncrement(&sc->request_count);
				assert(result > 1);
			}
			break;
		default:
			if (req->msgloop) {
				result = -EBUSY;
			} else if (req->from != NULL) {
				if (flag == ARequest_InQueueFront)
					list_add(&msg->entry, &req->request_list);
				else
					list_add_tail(&msg->entry, &req->request_list);
				result = 0;
			} else {
				req->from = msg;
				result = InterlockedIncrement(&sc->request_count);
				assert(result > 1);
			}
			break;
		}
	}
	LeaveCriticalSection(&req->lock);
	if (result <= 0)
		return result;

	AObjectAddRef(&sc->object);
	AMsgInit(&req->msg, msg->type, msg->data, msg->size);

	result = sc->stream->request(sc->stream, reqix, &req->msg);
	if (result != 0)
	{
		long ret = result;
		AMsgInit(msg, req->msg.type, req->msg.data, req->msg.size);
		if (req->msgloop && (result > 0)) {
			msg->done(msg, result);
			result = 0;
		}
		SyncRequestDispatch(req, ret, TRUE);
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
	if ((flag == ANotify_InQueueFront)
	 || (flag == ANotify_InQueueBack)) {
		head = &req->notify_list;
	} else {
		head = &req->request_list;
	}

	long result;
	EnterCriticalSection(&req->lock);
	if (flag == ARequest_MsgLoop) {
		if (!req->msgloop) {
			result = -ENODEV;
		} else if ((msg == NULL) || (msg == req->from)) {
			req->abortloop = 1;
			result = 0;
		} else {
			result = -EINVAL;
		}
	} else if (sc->status != stream_opened) {
		result = -EINTR;
	} else if (msg == req->from) {
		result = 0;
	} else {
		AMessage *pos;
		result = -ENODEV;
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
		long old_status = InterlockedCompareExchange((long volatile*)&sc->status, new_status, test_status);
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
	if (result != 0)
		SyncControlClosed(sc, req);
	return result;
}

AModule SyncControlModule = {
	"stream",
	"SyncControl",
	sizeof(SyncControl),
	NULL, NULL,
	&SyncControlCreate,
	&SyncControlRelease,
	NULL,
	0,

	&SyncControlOpen,
	&SyncControlSetOption,
	&SyncControlGetOption,
	&SyncControlRequest,
	&SyncControlCancel,
	&SyncControlClose,
};
