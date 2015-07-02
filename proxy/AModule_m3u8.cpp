#include "stdafx.h"
#include "../base/AModule.h"
#include "../base/SliceBuffer.h"
#include "../PVDClient/PvdNetCmd.h"
#include "../base/srsw.hpp"
#include "../base/async_operator.h"

extern AObject *rt;
extern AMessage rt_msg;

static const char *http_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text\r\n"
	"Content-Length: %d\r\n"
	"\r\n";

static const char *m3u8_file =
	"#EXTM3U\r\n"
	"#EXT-X-TARGETDURATION:10\r\n"
	"#EXT-X-MEDIA-SEQUENCE:%d\r\n"
	"#EXTINF:10,h264.mpg\r\n"
	"h264.mpg\r\n";

static USHORT media_sequence = 1;

static long ilen(long num) {
	int len = 1;
	while (num >= 10) {
		num /= 10;
		++len;
	}
	return len;
}

struct M3U8Proxy {
	AObject  object;
	AObject *client;
	SliceBuffer outbuf;
	AMessage    inmsg;
	AMessage    outmsg;
	AMessage   *from;
	srsw_queue<AMessage,64> frame_queue;
	async_operator timer;
	char        reply[512];
};
#define to_proxy(obj)  container_of(obj, M3U8Proxy, object)

static void M3U8ProxyRelease(AObject *object)
{
	M3U8Proxy *p = to_proxy(object);
	release_s(p->client, AObjectRelease, NULL);
	SliceFree(&p->outbuf);

	while (p->frame_queue.size() != 0) {
		AMessage &frame = p->frame_queue.front();
		if (frame.data != NULL)
			RTBufferFree(RTMsgGet(&frame));
		p->frame_queue.get_front();
	}
	free(p);
}

static long M3U8ProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	if (rt_msg.data == NULL)
		return -EFAULT;

	M3U8Proxy *p = (M3U8Proxy*)malloc(sizeof(M3U8Proxy));
	if (p == NULL)
		return -ENOMEM;

	extern AModule M3U8ProxyModule;
	AObjectInit(&p->object, &M3U8ProxyModule);
	p->client = parent;
	if (parent != NULL)
		AObjectAddRef(parent);
	SliceInit(&p->outbuf);
	p->frame_queue.reset();

	*object = &p->object;
	return 1;
}

static long M3U8ProxyOpen(AObject *object, AMessage *msg)
{
	M3U8Proxy *p = to_proxy(object);
	if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != sizeof(AObject)))
		return -EINVAL;

	release_s(p->client, AObjectRelease, NULL);
	p->client = (AObject*)msg->data;
	AObjectAddRef(p->client);
	return 1;
}

static long RTStreamDone(AMessage *msg, long result)
{
	M3U8Proxy *p = container_of(msg, M3U8Proxy, outmsg);
	if (result == 0) // on notify callback
	{
		if ((rt_msg.data == NULL) || (p->inmsg.done == NULL))
			return -1;

		STREAM_HEADER *sh = (STREAM_HEADER*)msg->data;
		if (ISMSHEAD(sh)) {
			if (!ISVIDEOFRAME(sh)) {
				AMsgInit(msg, AMsgType_Unknown, NULL, 0);
				return 0;
			}
			rt_msg.entry.next = (list_head*)MSHEAD_GETMSHSIZE(sh);
		} else if (Stream_IsHeaderFlag(sh)) {
			if (!Stream_IsVideoFrame(sh)) {
				AMsgInit(msg, AMsgType_Unknown, NULL, 0);
				return 0;
			}
			rt_msg.entry.next = (list_head*)sh->nHeaderSize;
		} else {
			AMsgInit(msg, AMsgType_Unknown, NULL, 0);
			return 0;
		}

		if (p->frame_queue.size() < p->frame_queue._capacity()) {
			RTBufferAddRef(RTMsgGet(&rt_msg));
			p->frame_queue.put_back(rt_msg);
		} else {
			//TRACE("drop stream frame(%d) size = %d...\n",
			//	msg->type&~AMsgType_Custom, msg->size);
		}
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}

	AMessage end_msg = { 0 };
	p->frame_queue.put_back(end_msg);
	TRACE("%p: quit recv stream...\n", p);

	result = p->from->done(p->from, result);
	return result;
}

static long M3U8AckDone(AMessage *msg, long result)
{
	M3U8Proxy *p = container_of(msg, M3U8Proxy, outmsg);
	if (result >= 0) {
		result = p->from->size;
	}
	result = p->from->done(p->from, result);
	return result;
}

static void M3U8ProxySendStream(async_operator *asop, int result)
{
	M3U8Proxy *p = container_of(asop, M3U8Proxy, timer);
	while (result >= 0)
	{
		if (p->frame_queue.size() == 0) {
			async_operator_timewait(&p->timer, NULL, 10);
			return;
		}

		AMessage &frame = p->frame_queue.front();
		if (frame.size == 0) {
			p->frame_queue.get_front();
			result = -1;
			break;
		}

		if (p->inmsg.type == 0) {
			static const char *ack =
				"HTTP/1.1 200 OK\r\n"
				"Transfer-Encoding: Chunked\r\n"
				"Content-Length: %d\r\n"
				"Content-Type: video/mpg\r\n"
				"\r\n";
			p->inmsg.type = AMsgType_Custom;
			p->inmsg.data = p->reply;
			p->inmsg.size = sprintf(p->inmsg.data, ack, frame.size-(long)frame.entry.next);
			result = p->client->request(p->client, ARequest_Input, &p->inmsg);
			if (result == 0)
				return;
			if (result < 0)
				break;
		}

		if (p->inmsg.type == AMsgType_Custom) {
			AMsgInit(&p->inmsg, AMsgType_Custom|1, frame.data+(long)frame.entry.next, frame.size-(long)frame.entry.next);
			result = p->client->request(p->client, ARequest_Input, &p->inmsg);
			if (result == 0)
				return;
			if (result < 0)
				break;
		}

		RTBufferFree(RTMsgGet(&frame));
		p->frame_queue.get_front();
		AMsgInit(&p->inmsg, 0, NULL, 0);
	}

	TRACE("%p: quit send stream!\n", p);
	p->inmsg.done = NULL;
	p->timer.callback = NULL;
	AObjectRelease(&p->object);
}

static long M3U8ProxyStreamDone(AMessage *msg, long result)
{
	M3U8Proxy *p = container_of(msg, M3U8Proxy, inmsg);
	M3U8ProxySendStream(&p->timer, result);
	return result;
}

static long M3U8ProxyRequest(AObject *object, long reqix, AMessage *msg)
{
	M3U8Proxy *p = to_proxy(object);
	if (reqix != ARequest_Input)
		return -ENOSYS;

	if (msg->data == NULL) {
		//AMsgInit(msg, AMsgType_Unknown, SliceResPtr(&p->outbuf), SliceResLen(&p->outbuf)-1);
		return -EACCES;
	}

	msg->data[msg->size] = '\0';
	TRACE("%p: request %d\n%s\n", p, reqix, msg->data);
	if (strstr(msg->data, ".m3u8") != NULL)
	{
		long seq = media_sequence;
		long m3u8_len = ilen(seq) + strlen(m3u8_file) - 2;
		long head_len = ilen(m3u8_len) + strlen(http_ack) - 2;

		p->outmsg.type = AMsgType_Unknown;
		p->outmsg.data = p->reply;
		p->outmsg.size = sprintf(p->outmsg.data, http_ack, m3u8_len);
		assert(p->outmsg.size == head_len);
		p->outmsg.size += sprintf(p->outmsg.data+head_len, m3u8_file, seq);
		assert(p->outmsg.size == head_len+m3u8_len);
		p->outmsg.done = &M3U8AckDone;
		p->from = msg;

		long result = p->client->request(p->client, ARequest_Input, &p->outmsg);
		if (result > 0) {
			result = msg->size;
		}
		return result;
	}
	//if (strstr(msg->data, ".ts") == NULL) {
	//	return -EACCES;
	//}

	//
	AMsgInit(&p->inmsg, 0, NULL, 0);
	p->inmsg.done = &M3U8ProxyStreamDone;
	p->timer.callback = &M3U8ProxySendStream;

	AMsgInit(&p->outmsg, AMsgType_Unknown, NULL, 0);
	p->outmsg.done = &RTStreamDone;
	p->from = msg;

	long result = rt->request(rt, ANotify_InQueueBack|0, &p->outmsg);
	if (result < 0) {
		AObjectRelease(&p->object);
	} else {
		AObjectAddRef(&p->object);
		async_operator_timewait(&p->timer, NULL, 10);
		result = 0;
	}
	return result;
}

static long M3U8ProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->type != AMsgType_Unknown)
		return -1;
	if (_strnicmp(msg->data, "GET ", 4) != 0)
		return -1;
	return 80;
}

AModule M3U8ProxyModule = {
	"proxy",
	"M3U8Proxy",
	sizeof(M3U8Proxy),
	NULL, NULL,
	&M3U8ProxyCreate,
	&M3U8ProxyRelease,
	&M3U8ProxyProbe,
	0,
	&M3U8ProxyOpen,
	NULL,
	NULL,
	&M3U8ProxyRequest,
	NULL,
	NULL,
};
