#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"


struct SyncRequest {
	struct SyncControl *sc;
	int                 reqix;
	struct list_head    entry;
	AMessage            msg;

	pthread_mutex_t     mutex;
	AMessage           *from;
	struct list_head    request_list;
	struct list_head    notify_list;
};
#define to_req(msg) container_of(msg, SyncRequest, msg)

#define syncreq_cache_count  32

struct SyncControl {
	AObject   object;
	AObject  *stream;
	int       alloc_self;
	int       open_result;
	AMessage *close_msg;

	long volatile status;
	long volatile request_count;
	struct list_head syncreq_list;
	SyncRequest     *syncreq_cache_list[syncreq_cache_count];
};
#define to_sc(obj) container_of(obj, SyncControl, object)

static SyncRequest* SyncRequestGet(SyncControl *sc, int reqix)
{
	if (reqix < syncreq_cache_count)
		return sc->syncreq_cache_list[reqix];

	//if (reqix > sc->object.reqix_count)
	//	return NULL;

	SyncRequest *req;
	list_for_each_entry(req, &sc->syncreq_list, SyncRequest, entry) {
		if (req->reqix == reqix)
			return req;
	}
	return NULL;
}

static int SyncRequestDone(AMessage *msg, int result);

static SyncRequest* SyncRequestNew(SyncControl *sc, int reqix)
{
	SyncRequest *req = (SyncRequest*)malloc(sizeof(SyncRequest));
	if (req == NULL)
		return NULL;

	req->sc = sc;
	req->reqix = reqix;
	list_add(&req->entry, &sc->syncreq_list);
	req->msg.done = &SyncRequestDone;
	INIT_LIST_HEAD(&req->msg.entry);

	pthread_mutex_init(&req->mutex, NULL);
	req->from = NULL;
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

	assert(sc->close_msg == NULL);
	if (sc->close_msg != NULL) {
		sc->close_msg->done(sc->close_msg, -EINTR);
		sc->close_msg = NULL;
	}

	//assert(list_empty(&sc->syncreq_list));
	while (!list_empty(&sc->syncreq_list)) {
		SyncRequest *req = list_pop_front(&sc->syncreq_list, SyncRequest, entry);
		if (req->from != NULL) {
			req->from->done(req->from, -EINTR);
			req->from = NULL;
		}
		AMsgListClear(&req->request_list, -EINTR);
		AMsgListClear(&req->notify_list, -EINTR);
		pthread_mutex_destroy(&req->mutex);
		free(req);
	}
	if (sc->alloc_self)
		free(sc);
}

static int SyncControlCreate(AObject **object, AObject *parent, AOption *option)
{
	SyncControl *sc = (SyncControl*)*object;
	if (sc == NULL) {
		sc = (SyncControl*)malloc(sizeof(SyncControl));
		if (sc == NULL)
			return -ENOMEM;

		*object = &sc->object;
		extern AModule SyncControlModule;
		//AObjectInit(&sc->object, &SyncControlModule);
		sc->object.init(&SyncControlModule);
		sc->alloc_self = TRUE;
	} else {
		sc->alloc_self = FALSE;
	}

	sc->stream = NULL;
	sc->open_result = 0;
	sc->close_msg = NULL;
	sc->status = AObject_Invalid;
	sc->request_count = 0;
	INIT_LIST_HEAD(&sc->syncreq_list);
	memset(sc->syncreq_cache_list, 0, sizeof(sc->syncreq_cache_list));

	int result = AObjectCreate(&sc->stream, parent, option, NULL);
	return result;
}

static int SyncControlOpenDone(AMessage *msg, int result);

static int SyncControlOpenStatus(SyncRequest *req, int &result)
{
	SyncControl *sc = req->sc;
	int old_status;
	switch (sc->status)
	{
	case AObject_Abort:
		result = -EINTR;
	case AObject_Opening:
		if (result >= 0)
		{
			/*while (sc->object.reqix_count < sc->stream->reqix_count) {
				if (SyncRequestNew(sc, sc->object.reqix_count) == NULL) {
					result = -ENOMEM;
					break;
				}
				++sc->object.reqix_count;
			}*/
		}
		AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);
		sc->open_result = result;
		if (result >= 0) {
			AMessage *msg = req->from;
			req->msg.done = &SyncRequestDone;
			req->from = NULL;
			old_status = InterlockedCompareExchange(&sc->status, AObject_Opened, AObject_Opening);
			if (old_status == AObject_Opening)
				return 1;

			assert(old_status == AObject_Abort);
			sc->open_result = -EINTR;
			req->msg.done = &SyncControlOpenDone;
			req->from = msg;
		}

		old_status = InterlockedExchange(&sc->status, AObject_Closing);
		assert((old_status == AObject_Opening) || (old_status == AObject_Abort));

		AMsgInit(&req->msg, AMsgType_Unknown, NULL, 0);
		result = sc->stream->close(&req->msg);
		if (result == 0)
			return 0;

	case AObject_Closing:
		result = InterlockedAdd(&sc->request_count, -1);
		assert(result == 0);
		result = sc->open_result;

		req->msg.done = &SyncRequestDone;
		req->from = NULL;
		old_status = InterlockedExchange(&sc->status, AObject_Closed);
		assert(old_status == AObject_Closing);
		return -1;

	default:
		assert(FALSE);
		result = -EACCES;
		return -1;
	}
}

static int SyncControlOpenDone(AMessage *msg, int result)
{
	SyncRequest *req = to_req(msg);
	msg = req->from;

	if (SyncControlOpenStatus(req, result) != 0)
		result = msg->done(msg, result);
	return result;
}

static int SyncControlOpen(AObject *object, AMessage *msg)
{
	SyncControl *sc = to_sc(object);

	int result = InterlockedCompareExchange(&sc->status, AObject_Opening, AObject_Invalid);
	if (result != AObject_Invalid) {
		if (result == AObject_Closed)
			result = InterlockedCompareExchange(&sc->status, AObject_Opening, AObject_Closed);
		if (result != AObject_Closed)
			return -EBUSY;
	}

	SyncRequest *req = SyncRequestGet(sc, 0);
	if (req == NULL) {
		req = SyncRequestNew(sc, 0);
		if (req == NULL)
			return -ENOMEM;
		//sc->object.reqix_count = 1;
	}

	result = InterlockedAdd(&sc->request_count, 1);
	assert(result == 1);

	AMsgInit(&req->msg, msg->type, msg->data, msg->size);
	req->msg.done = &SyncControlOpenDone;
	req->from = msg;

	result = sc->stream->open(&req->msg);
	if (result != 0)
		SyncControlOpenStatus(req, result);
	return result;
}

static int SyncControlSetOption(AObject *object, AOption *option)
{
	SyncControl *sc = to_sc(object);
	return sc->stream->setopt(option);
}

static int SyncControlGetOption(AObject *object, AOption *option)
{
	SyncControl *sc = to_sc(object);
	return sc->stream->getopt(option);
}

static void SyncControlClosed(SyncControl *sc, SyncRequest *req)
{
	assert(sc->close_msg == req->from);
	sc->close_msg = NULL;

	AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);
	req->msg.done = &SyncRequestDone;
	req->from = NULL;

	int old_status = InterlockedExchange(&sc->status, AObject_Closed);
	assert(old_status == AObject_Closing);
}

static int SyncControlCloseDone(AMessage *msg, int result)
{
	SyncRequest *req = to_req(msg);
	msg = req->from;
	SyncControlClosed(req->sc, req);
	result = msg->done(msg, result);
	return result;
}

static int SyncControlDoClose(SyncControl *sc, SyncRequest *req)
{
	SyncRequest *pos;
	list_for_each_entry(pos, &sc->syncreq_list, SyncRequest, entry) {
		AMsgListClear(&pos->request_list, -EINTR);
		AMsgListClear(&pos->notify_list, -EINTR);
	}

	AMsgInit(&req->msg, sc->close_msg->type, sc->close_msg->data, sc->close_msg->size);
	req->msg.done = &SyncControlCloseDone;
	req->from = sc->close_msg;

	return sc->stream->close(&req->msg);
}

static void SyncRequestDispatchRequest(SyncRequest *req, int result)
{
	SyncControl *sc = req->sc;
	AMessage *msg;

	struct list_head quit_list;
	INIT_LIST_HEAD(&quit_list);

	for (;;) {
		AMsgInit(req->from, req->msg.type, req->msg.data, req->msg.size);
		req->from->done(req->from, result);

		pthread_mutex_lock(&req->mutex);
		if (result >= 0) {
			msg = AMsgDispatch(&req->notify_list, &req->msg, &quit_list);
		} else {
			msg = NULL;
		}
		if (sc->status != AObject_Opened) {
			req->from = NULL;
			result = -EINTR;
		} else if (list_empty(&req->request_list)) {
			req->from = NULL;
			result = 0;
		} else {
			req->from = list_pop_front(&req->request_list, AMessage, entry);
			result = 1;
		}
		pthread_mutex_unlock(&req->mutex);

		if (msg != NULL) {
			msg->done(msg, 1);
		}
		while (!list_empty(&quit_list)) {
			msg = list_pop_front(&quit_list, AMessage, entry);
			msg->done(msg, -1);
		}
		if (result <= 0)
			break;

		AMsgInit(&req->msg, req->from->type, req->from->data, req->from->size);
		result = sc->stream->request(req->reqix, &req->msg);
		if (result == 0)
			return;
	}

	if (InterlockedAdd(&sc->request_count, -1) == 0) {
		result = SyncControlDoClose(sc, req);
		if (result != 0) {
			msg = sc->close_msg;
			SyncControlClosed(sc, req);
			msg->done(msg, result);
		}
	}
	AObjectRelease(&sc->object);
}

static int SyncRequestDone(AMessage *msg, int result)
{
	SyncRequest *req = to_req(msg);

	SyncRequestDispatchRequest(req, result);
	return result;
}

static int SyncRequestCancelDispath(SyncRequest *req, AMessage *from)
{
	AMessage *msg = NULL;
	struct list_head quit_list;
	INIT_LIST_HEAD(&quit_list);

	pthread_mutex_lock(&req->mutex);
	AMessage *pos;
	list_for_each_entry(pos, &req->notify_list, AMessage, entry)
	{
		AMsgInit(from, AMsgType_OtherMsg, pos, 0);
		int result = from->done(from, 0);
		if (result == 0)
			continue;

		if (result > 0) {
			list_del_init(&pos->entry);
			msg = pos;
			break;
		}

		pos = list_entry(pos->entry.prev, AMessage, entry);
		list_move_tail(pos->entry.next, &quit_list);
	}
	pthread_mutex_unlock(&req->mutex);

	if (msg != NULL) {
		msg->done(msg, 1);
	}
	while (!list_empty(&quit_list)) {
		msg = list_pop_front(&quit_list, AMessage, entry);
		msg->done(msg, -1);
	}
	return 1;
}

static int SyncControlRequest(AObject *object, int reqix, AMessage *msg)
{
	SyncControl *sc = to_sc(object);
	if (sc->status != AObject_Opened)
		return -ENOENT;

	int flag = (reqix & ~Aiosync_IndexMask);
	reqix = reqix & Aiosync_IndexMask;

	SyncRequest *req = SyncRequestGet(sc, reqix);
	if (req == NULL)
		return -ENOENT;

	if (flag == Aiosync_NotifyDispath)
		return AMsgDispatch2(&req->mutex, &req->notify_list, msg);

	int result;
	pthread_mutex_lock(&req->mutex);
	if (sc->status != AObject_Opened) {
		result = -EINTR;
	} else {
		switch (flag)
		{
		case Aiosync_NotifyFront:
			list_add(&msg->entry, &req->notify_list);
			result = 0;
			break;
		case Aiosync_NotifyBack:
			list_add_tail(&msg->entry, &req->notify_list);
			result = 0;
			break;
		default:
			if (req->from == NULL) {
				assert(list_empty(&req->request_list));
				req->from = msg;
				result = InterlockedAdd(&sc->request_count, 1);
				assert(result > 1);
			} else if (flag == Aiosync_RequestFront) {
				list_add(&msg->entry, &req->request_list);
				result = 0;
			} else {
				list_add_tail(&msg->entry, &req->request_list);
				result = 0;
			}
			break;
		}
	}
	pthread_mutex_unlock(&req->mutex);
	if (result <= 0)
		return result;

	AObjectAddRef(&sc->object);
	AMsgInit(&req->msg, msg->type, msg->data, msg->size);

	result = sc->stream->request(reqix, &req->msg);
	if (result != 0) {
		SyncRequestDispatchRequest(req, result);
		result = 0;
	}
	return result;
}

static int SyncControlCancel(AObject *object, int reqix, AMessage *msg)
{
	SyncControl *sc = to_sc(object);
	if (sc->status != AObject_Opened)
		return -ENOENT;

	int flag = (reqix & ~Aiosync_IndexMask);
	reqix = reqix & Aiosync_IndexMask;

	SyncRequest *req = SyncRequestGet(sc, reqix);
	if (req == NULL)
		return -ENOENT;

	if (flag == Aiosync_NotifyDispath)
		return SyncRequestCancelDispath(req, msg);

	struct list_head *head;
	if ((flag == Aiosync_NotifyFront)
	 || (flag == Aiosync_NotifyBack)) {
		head = &req->notify_list;
	} else {
		head = &req->request_list;
	}

	int result = 0;
	pthread_mutex_lock(&req->mutex);
	if (sc->status != AObject_Opened) {
		result = -EINTR;
	} else if (msg == NULL) {
		result = sc->stream->cancel(reqix, NULL);
	} else if (msg != req->from) {
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
	pthread_mutex_unlock(&req->mutex);
	return result;
}

static int SyncControlClose(AObject *object, AMessage *msg)
{
	SyncControl *sc = to_sc(object);
	if (msg == NULL) {
		if (sc->stream == NULL)
			return -ENOENT;
		return sc->stream->close(NULL);
	}

	int new_status = AObject_Abort;
	int test_status = AObject_Opening;
	for (;;) {
		int old_status = InterlockedCompareExchange(&sc->status, new_status, test_status);
		if ((old_status == AObject_Invalid) || (old_status == AObject_Closed))
			return -ENOENT;
		if ((old_status == AObject_Abort) || (old_status == AObject_Closing))
			return -EBUSY;
		if (old_status != test_status) {
			test_status = AObject_Opened;
			new_status = AObject_Closing;
			Sleep(0);
			continue;
		}
		if (old_status == AObject_Opening)
			return 1;
		assert(old_status == AObject_Opened);
		break;
	}

	SyncRequest *req;
	list_for_each_entry(req, &sc->syncreq_list, SyncRequest, entry) {
		pthread_mutex_lock(&req->mutex);
		pthread_mutex_unlock(&req->mutex);
	}

	sc->close_msg = msg;
	if (InterlockedAdd(&sc->request_count, -1) != 0)
		return 0;

	req = SyncRequestGet(sc, 0);
	int result = SyncControlDoClose(sc, req);
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

static auto_reg_t<SyncControlModule> auto_reg;
