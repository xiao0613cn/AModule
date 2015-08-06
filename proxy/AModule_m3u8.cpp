#include "stdafx.h"
#include "../base/AModule.h"
#include "../io/AModule_io.h"
#include "../PVDClient/PvdNetCmd.h"
#include "../base/srsw.hpp"
#include "../base/async_operator.h"
#ifdef __cplusplus
extern "C" {
#endif
#define __STDC_CONSTANT_MACROS
#include "libavformat/avformat.h"
#ifdef __cplusplus
};
#endif

extern AObject *rt;

static const char *m3u8_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text\r\n" //application/vnd.apple.mpegurl
	"Content-Length: %d\r\n"
	"\r\n";

static const char *m3u8_file =
	"#EXTM3U\r\n"
	"#EXT-X-TARGETDURATION:10\r\n"
	"#EXT-X-MEDIA-SEQUENCE:%d\r\n"
	"#EXTINF:10,h264.mp4\r\n"
	"h264.mp4\r\n";

static const char *media_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: video/mp4\r\n"
	"Content-Length: %d\r\n"
	"\r\n";

static CRITICAL_SECTION media_lock;
static DWORD     media_sequence = 0;
static list_head media_file;
static long      media_file_size;

static list_head tmp_media;
static long      tmp_media_size;
static ARefsBuf *tmp_buffer;
static long      tmp_offset;
static time_t    first_time;

AVFormatContext *tmp_avfx;
AVPacket         tmp_avpkt;

static AMessage  rt_msg;

static long ilen(long num) {
	int len = 1;
	while (num >= 10) {
		num /= 10;
		++len;
	}
	return len;
}

static void MediaListClear(struct list_head *list) {
	while (!list_empty(list)) {
		ARefsMsg *rm = list_first_entry(list, ARefsMsg, msg.entry);
		list_del_init(&rm->msg.entry);
		ARefsBufRelease(rm->buf);
	}
}

struct M3U8Proxy {
	AObject   object;
	AObject  *client;
	AMessage  outmsg;
	AMessage *from;

	char      reply[512];
	struct list_head reply_frame;
	ARefsMsg *reply_msg;
};
#define to_proxy(obj)  container_of(obj, M3U8Proxy, object)

static void M3U8ProxyRelease(AObject *object)
{
	M3U8Proxy *p = to_proxy(object);
	release_s(p->client, AObjectRelease, NULL);

	MediaListClear(&p->reply_frame);
	if (p->reply_msg != NULL) {
		ARefsBufRelease(p->reply_msg->buf);
		p->reply_msg = NULL;
	}
	free(p);
}

static long M3U8ProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	if (rt == NULL)
		return -EFAULT;

	M3U8Proxy *p = (M3U8Proxy*)malloc(sizeof(M3U8Proxy));
	if (p == NULL)
		return -ENOMEM;

	extern AModule M3U8ProxyModule;
	M3U8ProxyModule.init(NULL);

	AObjectInit(&p->object, &M3U8ProxyModule);
	p->client = parent;
	if (parent != NULL)
		AObjectAddRef(parent);

	INIT_LIST_HEAD(&p->reply_frame);
	p->reply_msg = NULL;

	*object = &p->object;
	return 1;
}

static long M3U8ProxyOpen(AObject *object, AMessage *msg)
{
	M3U8Proxy *p = to_proxy(object);
	if ((msg->type != AMsgType_Object)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	release_s(p->client, AObjectRelease, NULL);
	p->client = (AObject*)msg->data;
	AObjectAddRef(p->client);
	return 1;
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

static long OnMp4AckDone(M3U8Proxy *p)
{
	if (p->reply_msg != NULL) {
		ARefsBufRelease(p->reply_msg->buf);
		p->reply_msg = NULL;
	}
	while (!list_empty(&p->reply_frame))
	{
		p->reply_msg = list_first_entry(&p->reply_frame, ARefsMsg, msg.entry);
		list_del_init(&p->reply_msg->msg.entry);

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply_msg->data();
		p->outmsg.size = p->reply_msg->size;

		long result = p->client->request(p->client, Aio_RequestInput, &p->outmsg);
		if (result == 0)
			return 0;

		ARefsBufRelease(p->reply_msg->buf);
		p->reply_msg = NULL;
	}
	return p->from->size;
}

static long Mp4AckDone(AMessage *msg, long result)
{
	M3U8Proxy *p = container_of(msg, M3U8Proxy, outmsg);
	if (result >= 0) {
		result = OnMp4AckDone(p);
	}
	if (result != 0)
		result = p->from->done(p->from, result);
	return result;
}

static long M3U8ProxyRequest(AObject *object, long reqix, AMessage *msg)
{
	M3U8Proxy *p = to_proxy(object);
	if (reqix != Aio_RequestInput)
		return -ENOSYS;
	if (msg->data == NULL)
		return -EINVAL;

	msg->data[msg->size] = '\0';
	if (strnicmp(msg->data, "GET ", 4) != 0)
		return -EINVAL;
	TRACE(msg->data);

	const char *file_name = msg->data + 4;
	if (strnicmp(file_name, "/h264.m3u8 ", 11) == 0)
	{
		int m3u8_len = sprintf(p->reply+100, m3u8_file, media_sequence);
		int head_len = strlen(m3u8_ack) -2+ ilen(m3u8_len);
		_snprintf(p->reply+100-head_len, head_len, m3u8_ack, m3u8_len);

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply+100-head_len;
		p->outmsg.size = head_len + m3u8_len;
		p->outmsg.done = &M3U8AckDone;
		TRACE(p->outmsg.data);

		p->from = msg;
		long result = p->client->request(p->client, Aio_RequestInput, &p->outmsg);
		if (result > 0)
			result = p->outmsg.size;
		return result;
	}
	if (strnicmp(file_name, "/h264.mp4 ", 10) == 0)
	{
		EnterCriticalSection(&media_lock);
		list_splice_tail_init(&media_file, &p->reply_frame);
		long content_length = media_file_size;
		media_file_size = 0;
		LeaveCriticalSection(&media_lock);

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply;
		p->outmsg.size = sprintf(p->reply, media_ack, content_length);
		p->outmsg.done = &Mp4AckDone;
		TRACE(p->outmsg.data);

		p->from = msg;
		long result = p->client->request(p->client, Aio_RequestInput, &p->outmsg);
		if (result > 0)
			result = OnMp4AckDone(p);
		return result;
	}
	return -EACCES;
}

static long M3U8ProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->type != AMsgType_Unknown)
		return -1;
	if (_strnicmp(msg->data, "GET ", 4) != 0)
		return -1;
	return 80;
}

static long RTStreamDone(AMessage *msg, long result)
{
	if (result != 0)
		return result;

	if ((tmp_buffer == NULL) || (tmp_buffer->size < tmp_offset+sizeof(ARefsMsg)+msg->size)) {
		release_s(tmp_buffer, ARefsBufRelease, NULL);
		tmp_buffer = ARefsBufCreate(max(1024*1024, 10*msg->size));
		if (tmp_buffer == NULL)
			return -ENOMEM;
		tmp_offset = 0;
	}

	ARefsMsg *rm = (ARefsMsg*)(tmp_buffer->data + tmp_offset);
	time_t tm;

	// on notify callback
	if (ISMSHEAD(msg->data)) {
		MSHEAD *msh = (MSHEAD*)msg->data;
		if (!ISVIDEOFRAME(msh)) {
			AMsgInit(msg, AMsgType_Unknown, NULL, 0);
			return 0;
		}

		ARefsMsgInit(rm, AMsgType_Custom, tmp_buffer, tmp_offset+sizeof(ARefsMsg), MSHEAD_GETMSDSIZE(msh));
		memcpy(rm->data(), msg->data+MSHEAD_GETMSHSIZE(msh), rm->size);
		tm = msh->time_sec;
	}
	else if (Stream_IsValidFrame(msg->data, msg->size)) {
		STREAM_HEADER *sh = (STREAM_HEADER*)msg->data;
		if (!Stream_IsVideoFrame(sh)) {
			AMsgInit(msg, AMsgType_Unknown, NULL, 0);
			return 0;
		}

		ARefsMsgInit(rm, AMsgType_Custom, tmp_buffer, tmp_offset+sizeof(ARefsMsg), sh->nEncodeDataSize);
		memcpy(rm->data(), msg->data+sh->nHeaderSize, rm->size);

		STREAM_VIDEO_HEADER *vh = (STREAM_VIDEO_HEADER*)(sh + 1);
		tm = vh->nTimeStampLow;
	}
	else {
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}

	tmp_offset += sizeof(ARefsMsg) + rm->size;
	tmp_offset = _align_8bytes(tmp_offset);
	ARefsBufAddRef(tmp_buffer);
	list_add_tail(&rm->msg.entry, &tmp_media);
	tmp_media_size += rm->size;

	if (tmp_avfx != NULL) {
		av_init_packet(&tmp_avpkt);
		tmp_avpkt.data = (uint8_t*)rm->data();
		tmp_avpkt.size = rm->size;
		long ret = av_write_frame(tmp_avfx, &tmp_avpkt);
		TRACE("av_write_frame(%d) = %d.\n", rm->size, ret);
	}

	if (first_time == 0) {
		first_time = tm;
	} else if (first_time+10 < tm) {
		first_time = 0;
		LIST_HEAD(last_media);

		EnterCriticalSection(&media_lock);
		TRACE("free last media(%d) file size = %d.\n", media_sequence, media_file_size);
		++media_sequence;

		list_splice_tail_init(&media_file, &last_media);
		list_splice_tail_init(&tmp_media, &media_file);

		media_file_size = tmp_media_size;
		tmp_media_size = 0;
		LeaveCriticalSection(&media_lock);

		MediaListClear(&last_media);
	}

	AMsgInit(msg, AMsgType_Unknown, NULL, 0);
	return 0;
}

int tmp_avio_write(void *opaque, uint8_t *buf, int buf_size)
{
	TRACE("avio write = %d\n", buf_size);
	return buf_size;
}

static long M3U8ProxyInit(AOption *option)
{
	if ((rt == NULL) || (media_sequence != 0))
		return 0;

	AMsgInit(&rt_msg, AMsgType_Unknown, NULL, 0);
	rt_msg.done = &RTStreamDone;
	long result = rt->request(rt, Aiosync_NotifyBack|0, &rt_msg);
	TRACE("m3u8 stream register = %d.\n", result);
	if (result < 0)
		return 0;

	media_sequence = 1;
	InitializeCriticalSection(&media_lock);
	INIT_LIST_HEAD(&media_file);
	media_file_size = 0;

	INIT_LIST_HEAD(&tmp_media);
	tmp_media_size = 0;
	tmp_buffer = NULL;
	tmp_offset = 0;
	first_time = 0;

	av_register_all();
	tmp_avfx = NULL;

	long ret = avformat_alloc_output_context2(&tmp_avfx, NULL, "mpegts", NULL);
	if (ret < 0) {
		TRACE("avformat_alloc_output_context2(mpegts) = %d.\n", ret);
		return ret;
	}

	if (tmp_avfx->pb == NULL)
		tmp_avfx->pb = avio_alloc_context(NULL, 0, 1, NULL, NULL, &tmp_avio_write, NULL);

	ret = avformat_write_header(tmp_avfx, NULL);
	if (ret < 0) {
		TRACE("avformat_write_header() = %d.\n", ret);
	}
	return 1;
}

static void M3U8ProxyExit(void)
{
	if (media_sequence == 0)
		return;
	if (rt != NULL)
		rt->cancel(rt, Aiosync_NotifyBack|0, &rt_msg);
	first_time = 0;
	tmp_offset = 0;
	release_s(tmp_buffer, ARefsBufRelease, NULL);
	tmp_media_size = 0;
	MediaListClear(&tmp_media);

	MediaListClear(&media_file);
	DeleteCriticalSection(&media_lock);
	media_sequence = 0;
}

AModule M3U8ProxyModule = {
	"proxy",
	"M3U8Proxy",
	sizeof(M3U8Proxy),
	&M3U8ProxyInit,
	&M3U8ProxyExit,
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
