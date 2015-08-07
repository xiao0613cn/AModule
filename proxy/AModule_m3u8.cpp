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

extern AObject  *rt;
extern AMessage  rt_msg;
static AMessage  tmp_msg;

#define rt_m3u8   "h264.m3u8"
#define rt_name   "h264.ts"

#define pb_m3u8   "file.m3u8"
#define pb_name   "file.ts"

#define sec_per_file   10 // 10 seconds per file

static const char *m3u8_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text\r\n" //application/vnd.apple.mpegurl
	"Content-Length: %d\r\n"
	"\r\n";

static const char *m3u8_file =
	"#EXTM3U\r\n"
	"#EXT-X-MEDIA-SEQUENCE:%d\r\n"
	"#EXT-X-TARGETDURATION:%d\r\n"
	"#EXTINF:%d\r\n"
	"%s\r\n";

static const char *media_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: video/mp2ts\r\n"
	"Content-Length: %d\r\n"
	"\r\n";

struct media_file_t {
	long  content_length;
	long  nb_buffers;
	ARefsBuf *buffers[8];
};
static void media_file_release(media_file_t *mf) {
	long total_length = 0;
	while (mf->nb_buffers != 0) {
		--mf->nb_buffers;
		ARefsBuf *buf = mf->buffers[mf->nb_buffers];
		total_length += buf->size;
		ARefsBufRelease(buf);
	}
	//TRACE("free media, content length = %d, buffer size = %d.\n", mf->content_length, total_length);
	mf->content_length = 0;
}

static CRITICAL_SECTION rt_lock;
static DWORD        rt_seq = 0;
static media_file_t rt_media;

static media_file_t tmp_media;
static long         tmp_offset;
static int64_t      first_pts;
static media_file_t tmp_swap;

AVFormatContext *tmp_avfx;
AVPacket         tmp_avpkt;

//////////////////////////////////////////////////////////////////////////
struct M3U8Proxy {
	AObject   object;
	AObject  *client;
	AMessage  outmsg;
	AMessage *from;

	char      reply[512];
	media_file_t reply_file;
	long         reply_index;
};
#define to_proxy(obj)  container_of(obj, M3U8Proxy, object)

static void M3U8ProxyRelease(AObject *object)
{
	M3U8Proxy *p = to_proxy(object);
	release_s(p->client, AObjectRelease, NULL);
	media_file_release(&p->reply_file);

	free(p);
}

static long M3U8ProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	extern AModule M3U8ProxyModule;
	M3U8ProxyModule.init(NULL);
	if ((rt == NULL) || (rt_seq == 0))
		return -EFAULT;

	M3U8Proxy *p = (M3U8Proxy*)malloc(sizeof(M3U8Proxy));
	if (p == NULL)
		return -ENOMEM;

	AObjectInit(&p->object, &M3U8ProxyModule);
	p->client = parent;
	if (parent != NULL)
		AObjectAddRef(parent);
	memset(&p->reply_file, 0, sizeof(p->reply_file));
	p->reply_index = 0;

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
	while (p->reply_index < p->reply_file.nb_buffers)
	{
		ARefsBuf *buf = p->reply_file.buffers[p->reply_index];
		if (p->outmsg.data == buf->data) {
			assert(p->outmsg.size == buf->size);
			++p->reply_index;
			continue;
		}

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = buf->data;
		p->outmsg.size = buf->size;

		long result = p->client->request(p->client, Aio_RequestInput, &p->outmsg);
		if (result == 0)
			return 0;
	}
	media_file_release(&p->reply_file);
	p->reply_index = 0;

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
	if (strnicmp_c(msg->data, "GET ") != 0)
		return -EINVAL;
	OutputDebugStringA(msg->data);
	fputs(msg->data, stdout);

	const char *file_name = msg->data + 4;
	if (strnicmp_c(file_name, "/"rt_m3u8" ") == 0)
	{
		int m3u8_len = sprintf(p->reply+100, m3u8_file, rt_seq,
				sec_per_file, sec_per_file, rt_name);
		int head_len = sprintf(p->reply, m3u8_ack, m3u8_len);
		memmove(p->reply+head_len, p->reply+100, m3u8_len+1);

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply;
		p->outmsg.size = head_len + m3u8_len;
		p->outmsg.done = &M3U8AckDone;
		OutputDebugStringA(p->outmsg.data);
		fputs(p->outmsg.data, stdout);

		p->from = msg;
		long result = p->client->request(p->client, Aio_RequestInput, &p->outmsg);
		if (result > 0)
			result = p->outmsg.size;
		return result;
	}
	if (strnicmp_c(file_name, "/"rt_name" ") == 0)
	{
		media_file_release(&p->reply_file);
		p->reply_index = 0;

		if (rt_media.nb_buffers != 0) {
			EnterCriticalSection(&rt_lock);
			p->reply_file = rt_media;
			for (long ix = 0; ix < rt_media.nb_buffers; ++ix)
				ARefsBufAddRef(rt_media.buffers[ix]);
			LeaveCriticalSection(&rt_lock);
		}

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply;
		p->outmsg.size = sprintf(p->reply, media_ack, p->reply_file.content_length);
		p->outmsg.done = &Mp4AckDone;
		OutputDebugStringA(p->outmsg.data);
		fputs(p->outmsg.data, stdout);

		p->from = msg;
		long result = p->client->request(p->client, Aio_RequestInput, &p->outmsg);
		if (result > 0)
			result = OnMp4AckDone(p);
		return result;
	}
	if (strnicmp_c(file_name, "/"pb_name" ") == 0)
	{
	}
	return -EACCES;
}

//////////////////////////////////////////////////////////////////////////
static long M3U8ProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->type != AMsgType_Unknown)
		return -1;
	if (strnicmp_c(msg->data, "GET ") != 0)
		return -1;
	return 80;
}

static void RTStreamPush(void)
{
	TRACE("free last media(%d), size = %d.\n", rt_seq, rt_media.content_length);

	EnterCriticalSection(&rt_lock);
	++rt_seq;
	tmp_swap = rt_media;
	rt_media = tmp_media;
	LeaveCriticalSection(&rt_lock);

	media_file_release(&tmp_swap);
	memset(&tmp_media, 0, sizeof(tmp_media));
	first_pts = 0;
}

static int tmp_avio_write(void *opaque, uint8_t *data, int size)
{
	//TRACE("avio write = %d\n", buf_size);
	ARefsBuf *buf = tmp_media.buffers[tmp_media.nb_buffers];
	if ((buf != NULL) && (buf->size < tmp_offset+size))
	{
		buf->size = tmp_offset;
		buf = NULL;
		if (++tmp_media.nb_buffers >= _countof(tmp_media.buffers)) {
			RTStreamPush();
		}
	}
	if (buf == NULL) {
		buf = ARefsBufCreate(max(1024*1024, 10*size));
		if (buf == NULL)
			return -ENOMEM;
		tmp_media.buffers[tmp_media.nb_buffers] = buf;
		tmp_offset = 0;
	}

	memcpy(buf->data+tmp_offset, data, size);
	tmp_offset += size;
	tmp_media.content_length += size;
	return size;
}

static long RTStreamDone(AMessage *msg, long result)
{
	if (result != 0)
		return result;

	// on notify callback
	if (ISMSHEAD(msg->data)) {
		MSHEAD *msh = (MSHEAD*)msg->data;
		if (!ISVIDEOFRAME(msh)) {
			AMsgInit(msg, AMsgType_Unknown, NULL, 0);
			return 0;
		}

		av_init_packet(&tmp_avpkt);
		tmp_avpkt.data = (uint8_t*)msg->data + MSHEAD_GETMSHSIZE(msh);
		tmp_avpkt.size = min(msg->size-MSHEAD_GETMSHSIZE(msh), MSHEAD_GETMSDSIZE(msh));
		if (ISKEYFRAME(msh))
			tmp_avpkt.flags |= AV_PKT_FLAG_KEY;

		tmp_avpkt.pts = msh->time_sec*1000 + msh->time_msec*10;
	}
	else if (Stream_IsValidFrame(msg->data, msg->size)) {
		STREAM_HEADER *sh = (STREAM_HEADER*)msg->data;
		if (!Stream_IsVideoFrame(sh)) {
			AMsgInit(msg, AMsgType_Unknown, NULL, 0);
			return 0;
		}

		av_init_packet(&tmp_avpkt);
		tmp_avpkt.data = (uint8_t*)msg->data + sh->nHeaderSize;
		tmp_avpkt.size = min(msg->size-sh->nHeaderSize, sh->nEncodeDataSize);
		if (sh->nFrameType == STREAM_FRAME_VIDEO_I)
			tmp_avpkt.flags |= AV_PKT_FLAG_KEY;

		STREAM_VIDEO_HEADER *vh = (STREAM_VIDEO_HEADER*)(sh + 1);
		if (sh->nHeaderSize >= sizeof(*sh)+sizeof(*vh)) {
			tmp_avpkt.pts = ((int64_t(vh->nTimeStampHight)<<32) + vh->nTimeStampLow)*1000
			              + vh->nTimeStampMillisecond;
		}
	}
	else {
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}

	if (first_pts == 0) {
		first_pts = tmp_avpkt.pts;
	} else if (first_pts+10*1000 < tmp_avpkt.pts) {
		ARefsBuf *buf = tmp_media.buffers[tmp_media.nb_buffers];
		buf->size = tmp_offset;
		tmp_media.nb_buffers++;
		RTStreamPush();
	}

	if (tmp_avfx != NULL) {
		//AVStream *s = tmp_avfx->streams[tmp_avpkt.stream_index];
		//if (s->first_dts == 0)
		//	s->first_dts = tmp_avpkt.pts;

		//tmp_avpkt.dts = tmp_avpkt.pts;
		tmp_avpkt.pts *= 90;
		long ret = av_write_frame(tmp_avfx, &tmp_avpkt);
		if (ret < 0)
			TRACE("av_write_frame(%d) = %d.\n", tmp_avpkt.size, ret);
	}
#if 0
	tmp_avio_write(NULL, tmp_avpkt.data, tmp_avpkt.size);
#endif
	AMsgInit(msg, AMsgType_Unknown, NULL, 0);
	return 0;
}

static char log_buf[BUFSIZ];
static void av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	int len = _vsnprintf(log_buf, BUFSIZ-2, fmt, vl);
	log_buf[len++] = '\n';
	log_buf[len] = '\0';
	OutputDebugStringA(log_buf);
	fputs(log_buf, stdout);
}

static long M3U8ProxyInit(AOption *option)
{
	if ((rt == NULL) || (rt_seq != 0))
		return 0;

	AMsgInit(&tmp_msg, AMsgType_Unknown, NULL, 0);
	tmp_msg.done = &RTStreamDone;

	long result = rt->request(rt, Aiosync_NotifyBack|0, &tmp_msg);
	TRACE("m3u8 stream register = %d.\n", result);
	if (result < 0)
		return 0;

	rt_seq = 1;
	InitializeCriticalSection(&rt_lock);
	memset(&rt_media, 0, sizeof(rt_media));

	memset(&tmp_media, 0, sizeof(tmp_media));
	tmp_offset = 0;
	first_pts = 0;

	float a = 1.0;
	av_log_set_level(AV_LOG_MAX_OFFSET);
	av_log_set_callback(&av_log_callback);
	av_register_all();
	tmp_avfx = NULL;

	AVFormatContext *oc = NULL;
	long ret = avformat_alloc_output_context2(&oc, NULL, "mpegts", NULL);
	if (ret < 0) {
		TRACE("avformat_alloc_output_context2(mpegts) = %d.\n", ret);
		return ret;
	}

	oc->flags |= AVFMT_FLAG_NOBUFFER;
	oc->oformat->flags |= AVFMT_NOFILE|AVFMT_NODIMENSIONS|AVFMT_ALLOW_FLUSH;
	if (oc->pb == NULL) {
		oc->pb = avio_alloc_context(NULL, 0, 1, NULL, NULL, &tmp_avio_write, NULL);
		oc->pb->direct = TRUE;
	}

	AVCodec *avc = avcodec_find_encoder(AV_CODEC_ID_H264);//avcodec_find_decoder_by_name("h264");

	AVStream *s = avformat_new_stream(oc, avc);
	if (s != NULL) {
		AVCodecContext *codec = s->codec;
		ret = avcodec_get_context_defaults3(codec, avc);
		if (ret < 0) {
			TRACE("avcodec_get_context_defaults3(%s) = %d.\n", avc->name, ret);
		}

		codec->codec = avc;
		codec->codec_id = avc->id;
		codec->flags2 |= CODEC_FLAG2_NO_OUTPUT;
		codec->time_base.num = 1;
		codec->time_base.den = 90000;
		//codec->width = 704;
		//codec->height = 576;
		//codec->gop_size = 30;
		codec->pix_fmt = AV_PIX_FMT_YUV420P;

		ret = avcodec_open2(codec, avc, NULL);
		if (ret < 0) {
			TRACE("avcodec_open2(%s) = %d.\n", avc->name, ret);
		}

		s->id = 1;
		s->time_base = codec->time_base;
		//AVRational fps = { 30, 1 };
		//av_stream_set_r_frame_rate(s, fps);
		av_dump_format(oc, s->index, NULL, TRUE);
	}

	ret = avformat_write_header(oc, NULL);
	if (ret < 0) {
		TRACE("avformat_write_header() = %d.\n", ret);
	}
	tmp_avfx = oc;
	return 1;
}

static void M3U8ProxyExit(void)
{
	if (rt_seq == 0)
		return;
	if (rt != NULL)
		rt->cancel(rt, Aiosync_NotifyBack|0, &tmp_msg);
	first_pts = 0;
	tmp_offset = 0;
	media_file_release(&tmp_media);

	media_file_release(&rt_media);
	DeleteCriticalSection(&rt_lock);
	rt_seq = 0;
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
