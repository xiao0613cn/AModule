#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../PVDClient/PvdNetCmd.h"
#include "../base/srsw.hpp"

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
static AMessage  work_msg;
static AOperator work_opt;

#define live_m3u8     "h264.m3u8"
#define live_prefix   "h264_"

#define sec_per_file   5

static const char *m3u8_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: application/x-mpegURL\r\n" //application/vnd.apple.mpegurl\r\n" // text\r\n" //
	"Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n"
	"\r\n";

static const char *m3u8_head =
	"#EXTM3U\r\n"
	"#EXT-X-TARGETDURATION:%d\r\n"
	"#EXT-X-VERSION:3\r\n"
	"#EXT-X-MEDIA-SEQUENCE:%d\r\n";

static const char *media_datetime =
	"#EXT-X-PROGRAM-DATE-TIME:%Y-%m-%dT%H:%M:%S\r\n"
	"#EXT-X-DISCONTINUITY\r\n";

static const char *media_segment =
	"#EXTINF:%.3f,%d\r\n"
	"%s%d.ts\r\n";

static const char *media_ack =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: video/mp2t\r\n"
	"Cache-Control: no-cache\r\n"
	"Content-Length: %d\r\n"
	"\r\n";

static const char *media_404 =
	"HTTP/1.1 404 Not Found\r\n"
	"\r\n";

struct media_file_t {
	DWORD media_sequence;
	DWORD target_duration;
	//struct tm program_datetime;
	int   content_length;
	int   nb_buffers;
	ARefsBuf *buffers[16];
};
static void media_file_release(media_file_t *mf) {
	int total_length = 0;
	while (mf->nb_buffers != 0) {
		--mf->nb_buffers;
		ARefsBuf *buf = mf->buffers[mf->nb_buffers];
		total_length += buf->size;
		ARefsBufRelease(buf);
	}
	//TRACE("free media, content length = %d, buffer size = %d.\n", mf->content_length, total_length);
	mf->content_length = 0;
	//memset(&mf->program_datetime, 0, sizeof(mf->program_datetime));
	mf->target_duration = 0;
	mf->media_sequence = 0;
}

static pthread_mutex_t rt_mutex;
static DWORD        rt_seq = 0;
static media_file_t rt_media[3];

static media_file_t tmp_media;
static int          tmp_offset;
static media_file_t tmp_swap;

AVFormatContext *tmp_avfx;
AVPacket         tmp_avpkt;
static int64_t   pts_offset;

AVOutputFormat  *mpegts_ofmt;
AVCodec         *h264_codec;
int              vs_index;

static int avformat_open_output(AVFormatContext **oc, AVOutputFormat *ofmt, void *opaque, int(*write_packet)(void*,uint8_t*,int))
{
	int ret = avformat_alloc_output_context2(oc, ofmt, NULL, NULL);
	if (ret < 0) {
		TRACE("avformat_alloc_output_context2(mpegts) = %d.\n", ret);
		return ret;
	}

	(*oc)->flags |= AVFMT_FLAG_NOBUFFER;
	(*oc)->oformat->flags |= AVFMT_NOFILE|AVFMT_NODIMENSIONS|AVFMT_ALLOW_FLUSH;
	if ((*oc)->pb == NULL) {
		(*oc)->pb = avio_alloc_context(NULL, 0, 1, opaque, NULL, write_packet, NULL);
		(*oc)->pb->direct = TRUE;
	}
	return 0;
}

static void avformat_close_output(AVFormatContext *oc)
{
	if (oc->flags & AVFMT_NOFILE)
		avio_closep(&oc->pb);
	avformat_free_context(oc);
}

static int avformat_new_output_stream(AVFormatContext *oc, const AVCodec *codec, AVCodecContext *copy_from)
{
	if ((codec == NULL) && (copy_from != NULL))
		codec = copy_from->codec;

	AVStream *s = avformat_new_stream(oc, codec);
	if (s == NULL)
		return -ENOMEM;

	AVCodecContext *ctx = s->codec;
	if (copy_from == NULL) {
		int ret = avcodec_get_context_defaults3(ctx, codec);
		if (ret < 0) {
			TRACE("avcodec_get_context_defaults3(%s) = %d.\n",
				codec->name, ret);
			return ret;
		}

		ctx->codec = codec;
		ctx->codec_id = codec->id;
		ctx->flags2 |= CODEC_FLAG2_NO_OUTPUT;
		ctx->time_base.num = 1;
		ctx->time_base.den = 90000;
		//ctx->ticks_per_frame = 2;
		//codec->width = 704;
		//codec->height = 576;
		//codec->gop_size = 30;
		ctx->pix_fmt = AV_PIX_FMT_YUV420P;

		//ret = avcodec_open2(codec, avc, NULL);
		//if (ret < 0) {
		//	TRACE("avcodec_open2(%s) = %d.\n", avc->name, ret);
		//}
	} else {
		int ret = avcodec_copy_context(ctx, copy_from);
		if (ret < 0) {
			TRACE("avcodec_copy_context(%s, %s) = %d.\n",
				codec->name, copy_from->codec->name, ret);
			return ret;
		}
	}

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	//s->id = s->index + 1;
	s->time_base = ctx->time_base;

	s->avg_frame_rate.num = 30;
	s->avg_frame_rate.den = 1;
	av_dump_format(oc, s->index, NULL, TRUE);
	return s->index;
}

static int m3u8_file_build(char *content, int size, DWORD &last_seq)
{
	int len = 0;
	pthread_mutex_lock(&rt_mutex);
	for (int ix = 1; ix < _countof(rt_media); ++ix)
	{
		media_file_t *file = &rt_media[ix];
		if ((file->media_sequence == 0) || (file->content_length == 0))
			continue;

		if (len == 0) {
			len += sprintf_s(content+len, size-len-1, m3u8_head, sec_per_file, file->media_sequence);
			if (last_seq < file->media_sequence)
				len += sprintf_s(content+len, size-len-1, "#EXT-X-DISCONTINUITY\r\n");
		}
		//len += strftime(content+len, size-len, media_datetime, &file->program_datetime);
		len += sprintf_s(content+len, size-len-1, media_segment, file->target_duration/1000.0f, file->media_sequence,
			live_prefix, file->media_sequence);
		last_seq = file->media_sequence;
	}
	pthread_mutex_unlock(&rt_mutex);
	return len;
}

//////////////////////////////////////////////////////////////////////////
static long volatile alloc_count = 0;

struct M3U8Proxy {
	AObject   object;
	AObject  *client;
	AMessage  outmsg;
	AMessage *from;

	char      reply[512];
	media_file_t reply_file;
	int          reply_index;
	DWORD        last_seq;
	AVFormatContext *file_inctx;
	AVFormatContext *file_outctx;
};
#define to_proxy(obj)  container_of(obj, M3U8Proxy, object)

static void M3U8ProxyRelease(AObject *object)
{
	M3U8Proxy *p = to_proxy(object);
	release_s(p->client, AObjectRelease, NULL);
	media_file_release(&p->reply_file);

	if (p->file_inctx != NULL) avformat_close_input(&p->file_inctx);
	release_s(p->file_outctx, avformat_close_output, NULL);

	free(p);
	long ret = InterlockedAdd(&alloc_count, -1);
	TRACE("m3u8 proxy alloc count = %d.\n", ret);
}

static int M3U8ProxyCreate(AObject **object, AObject *parent, AOption *option)
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
	p->last_seq = 0;
	p->file_inctx = NULL;
	p->file_outctx = NULL;

	InterlockedAdd(&alloc_count, 1);
	*object = &p->object;
	return 1;
}

static int M3U8ProxyOpen(AObject *object, AMessage *msg)
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

static int M3U8AckDone(AMessage *msg, int result)
{
	M3U8Proxy *p = container_of(msg, M3U8Proxy, outmsg);
	if (result >= 0) {
		result = p->from->size;
	}
	result = p->from->done(p->from, result);
	return result;
}

static int OnMp4AckDone(M3U8Proxy *p)
{
	while (p->reply_index < p->reply_file.nb_buffers)
	{
		ARefsBuf *buf = p->reply_file.buffers[p->reply_index];
		if (p->outmsg.data == buf->data) {
			assert(p->outmsg.size == buf->size);
			//TRACE("send ts file(%d), size = %d, data = %s.\n", p->reply_file.media_sequence,
			//	p->outmsg.size, p->outmsg.data);
			++p->reply_index;
			continue;
		}

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = buf->data;
		p->outmsg.size = buf->size;

		int result = p->client->request(p->client, Aio_Input, &p->outmsg);
		if (result <= 0)
			return result;
	}

	p->last_seq = p->reply_file.media_sequence;
	TRACE("send ts file(%d) size = %d, nb_buffers = %d.\n", p->reply_file.media_sequence,
		p->reply_file.content_length, p->reply_file.nb_buffers);
	media_file_release(&p->reply_file);
	p->reply_index = 0;

	return p->from->size;
}

static int Mp4AckDone(AMessage *msg, int result)
{
	M3U8Proxy *p = container_of(msg, M3U8Proxy, outmsg);
	if (result >= 0) {
		result = OnMp4AckDone(p);
	}
	if (result != 0)
		result = p->from->done(p->from, result);
	return result;
}

static int M3U8OutputFile(void *opaque, uint8_t *data, int size)
{
	M3U8Proxy *p = (M3U8Proxy*)opaque;
	ARefsBuf *buf = NULL;
	int *offset;

	if (p->reply_file.nb_buffers != 0) {
		buf = p->reply_file.buffers[p->reply_file.nb_buffers-1];
		offset = (int*)(buf->data+buf->size-sizeof(int));
		if (*offset+size+sizeof(int) > buf->size) {
			buf->size = *offset;
			buf = NULL;
		}
	}
	if (buf == NULL) {
		if (p->reply_file.nb_buffers >= _countof(p->reply_file.buffers))
			return -ENOMEM;

		buf = ARefsBufCreate(1024*1024, NULL, NULL);
		if (buf == NULL)
			return -ENOMEM;

		p->reply_file.buffers[p->reply_file.nb_buffers] = buf;
		p->reply_file.nb_buffers++;

		offset = (int*)(buf->data+buf->size-sizeof(int));
		*offset = 0;
	}
	memcpy(buf->data+*offset, data, size);
	*offset += size;
	p->reply_file.content_length += size;
	return size;
}

static int M3U8OpenFile(M3U8Proxy *p, const char *file_name)
{
#if 0
	HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return -EBADF;

	BY_HANDLE_FILE_INFORMATION fi;
	memset(&fi, 0, sizeof(fi));
	GetFileInformationByHandle(file, &fi);

	ARefsBuf *buf = ARefsBufCreate(fi.nFileSizeLow);
	DWORD tx = 0;
	ReadFile(file, buf->data, fi.nFileSizeLow, &tx, NULL);
	CloseHandle(file);
	buf->size = tx;

	p->reply_file.buffers[0] = buf;
	p->reply_file.nb_buffers = 1;
	p->reply_file.content_length = tx;
#else
	int ret = avformat_open_input(&p->file_inctx, file_name, NULL, NULL);
	if (ret < 0) {
		TRACE("avformat_open_input(%s) = %d.\n", file_name, ret);
		return ret;
	}

	ret = avformat_find_stream_info(p->file_inctx, NULL);
	if (ret < 0) {
		TRACE("avformat_find_stream_info(%s) = %d.\n", file_name, ret);
		return ret;
	}

	av_dump_format(p->file_inctx, 0, file_name, FALSE);

	//
	ret = avformat_open_output(&p->file_outctx, mpegts_ofmt, p, &M3U8OutputFile);
	if (ret < 0) {
		TRACE("avformat_open_output(%s) = %d.\n", file_name, ret);
		return ret;
	}

	for (int ix = 0; ix < p->file_inctx->nb_streams; ++ix) {
		AVStream *is = p->file_inctx->streams[ix];
		ret = avformat_new_output_stream(p->file_outctx, NULL, is->codec);
		if (ret < 0)
			return ret;
	}

	ret = avformat_write_header(p->file_outctx, NULL);
	if (ret < 0) {
		TRACE("avformat_write_header(%s) = %d.\n", file_name, ret);
		return ret;
	}

	AVPacket pkt;
	for (;;) {
		av_init_packet(&pkt);

		ret = av_read_frame(p->file_inctx, &pkt);
		if (ret < 0) {
			TRACE("av_read_frame(%s) = %d.\n", file_name, ret);
			break;
		}

		AVStream *is = p->file_inctx->streams[pkt.stream_index];
		if (is->codec->codec_type != AVMEDIA_TYPE_VIDEO) {
			av_free_packet(&pkt);
			continue;
		}

		AVStream *os = p->file_outctx->streams[pkt.stream_index];
		pkt.pts = av_rescale_q_rnd(pkt.pts, is->time_base, os->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, is->time_base, os->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, is->time_base, os->time_base);
		pkt.pos = -1;

		ret = av_write_frame(p->file_outctx, &pkt);
		if (ret < 0) {
			TRACE("av_write_frame(%s) = %d.\n", file_name, ret);
			break;
		}
		av_free_packet(&pkt);
	}

	av_write_trailer(p->file_outctx);
	avformat_close_output(p->file_outctx);
	p->file_outctx = NULL;

	avformat_close_input(&p->file_inctx);
	if (p->reply_file.nb_buffers != 0) {
		ARefsBuf *buf = p->reply_file.buffers[p->reply_file.nb_buffers-1];
		int *offset = (int*)(buf->data+buf->size-sizeof(int));
		if (*offset != 0) {
			buf->size = *offset;
		} else {
			ARefsBufRelease(buf);
			p->reply_file.nb_buffers--;
		}
	}
	return -EACCES;
#endif
}

static int M3U8ProxyRequest(AObject *object, int reqix, AMessage *msg)
{
	M3U8Proxy *p = to_proxy(object);
	if (reqix != Aio_Input)
		return -ENOSYS;
	if (msg->data == NULL)
		return -EINVAL;

	msg->data[msg->size] = '\0';
	OutputDebugStringA(msg->data);
	fputs(msg->data, stdout);

	char *file_name = msg->data + sizeof("GET /")-1;
	if (strnicmp_c(file_name, live_m3u8" HTTP/") == 0)
	{
		char *content = p->reply + 200;
		int m3u8_len = m3u8_file_build(content, sizeof(p->reply)-200, p->last_seq);
		if (m3u8_len <= 0)
			return -EACCES;

		int head_len = sprintf_s(p->reply, 200, m3u8_ack, m3u8_len);
		memmove(p->reply+head_len, content, m3u8_len+1);

		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply;
		p->outmsg.size = head_len + m3u8_len;
		p->outmsg.done = &M3U8AckDone;
		OutputDebugStringA(p->outmsg.data);
		fputs(p->outmsg.data, stdout);

		p->from = msg;
		int result = p->client->request(p->client, Aio_Input, &p->outmsg);
		if (result > 0)
			result = p->outmsg.size;
		return result;
	}
	if (strnicmp_c(file_name, live_prefix) == 0)
	{
		media_file_release(&p->reply_file);
		p->reply_index = 0;
		DWORD seq = atoi(file_name+sizeof(live_prefix)-1);

		pthread_mutex_lock(&rt_mutex);
		for (int ix = 0; ix < _countof(rt_media); ++ix)
		{
			media_file_t *file = &rt_media[ix];
			if (file->media_sequence != seq)
				continue;

			memcpy(&p->reply_file, file, sizeof(media_file_t));
			for (int ix = 0; ix < file->nb_buffers; ++ix)
				ARefsBufAddRef(file->buffers[ix]);
			break;
		}
		if ((p->reply_file.content_length == 0) && (rt_media[0].content_length != 0)) {
			memcpy(&p->reply_file, &rt_media[0], sizeof(media_file_t));
			for (int ix = 0; ix < rt_media[0].nb_buffers; ++ix)
				ARefsBufAddRef(rt_media[0].buffers[ix]);
		}
		pthread_mutex_unlock(&rt_mutex);
_reply:
		if (p->reply_file.content_length != 0) {
			p->outmsg.size = sprintf_s(p->reply, media_ack, p->reply_file.content_length);
		} else {
			p->outmsg.size = sprintf_s(p->reply, media_404);
		}
		p->outmsg.type = AMsgType_Custom;
		p->outmsg.data = p->reply;
		p->outmsg.done = &Mp4AckDone;
		OutputDebugStringA(p->outmsg.data);
		fputs(p->outmsg.data, stdout);

		p->from = msg;
		int result = p->client->request(p->client, Aio_Input, &p->outmsg);
		if (result > 0)
			result = OnMp4AckDone(p);
		return result;
	}

	char *end = strchr(file_name, ' ');
	if (end == NULL)
		return -EACCES;

	*end++ = '\0';
	if ((strnicmp_c(end, "HTTP/1.0\r\n") != 0)
	 && (strnicmp_c(end, "HTTP/1.1\r\n") != 0))
		return msg->size;

	M3U8OpenFile(p, file_name);
	goto _reply;
}

//////////////////////////////////////////////////////////////////////////
static void RTStreamPush(void)
{
	pthread_mutex_lock(&rt_mutex);
	tmp_media.media_sequence = ++rt_seq;

	bool push = false;
	for (int ix = 0; ix < _countof(rt_media); ++ix) {
		media_file_t *file = &rt_media[ix];
		if (file->media_sequence == 0) {
			memcpy(file, &tmp_media, sizeof(media_file_t));
			push = true;
			break;
		}
	}
	if (!push) {
		tmp_swap = rt_media[0];
		memmove(&rt_media[0], &rt_media[1], sizeof(media_file_t)*(_countof(rt_media)-1));
		rt_media[_countof(rt_media)-1] = tmp_media;
	}
	pthread_mutex_unlock(&rt_mutex);
#if 0
	if (tmp_swap.content_length != 0) {
		char file_name[BUFSIZ];
		_snprintf(file_name, BUFSIZ, "./html/file/file%d.ts", tmp_swap.media_sequence);

		HANDLE file = CreateFileA(file_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE)
		{
			for (int ix = 0; ix < tmp_swap.nb_buffers; ++ix) {
				ARefsBuf *buf = tmp_swap.buffers[ix];
				DWORD tx = 0;
				WriteFile(file, buf->data, buf->size, &tx, NULL);
			}
			CloseHandle(file);
		}
	}
#endif
	TRACE("free last media(%d), size = %d.\n", tmp_swap.media_sequence, tmp_swap.content_length);
	media_file_release(&tmp_swap);
	memset(&tmp_media, 0, sizeof(tmp_media));
}

static int tmp_avio_write(void *opaque, uint8_t *data, int size)
{
	//TRACE("avio write = %d\n", buf_size);
	ARefsBuf *buf = NULL;
	if (tmp_media.nb_buffers != 0)
	{
		buf = tmp_media.buffers[tmp_media.nb_buffers-1];
		if (buf->size < tmp_offset+size)
		{
			buf->size = tmp_offset;
			buf = NULL;
		}
	}
	if (buf == NULL) {
		if (tmp_media.nb_buffers >= _countof(tmp_media.buffers)) {
			TRACE("ts stream(%d) out of memory.\n", tmp_media.content_length);
			RTStreamPush();
		}

		buf = ARefsBufCreate(max(1024*1024, 10*size), NULL, NULL);
		if (buf == NULL)
			return -ENOMEM;

		//if (tmp_media.nb_buffers == 0) {
		//	time_t cur_time = time(NULL);
		//	tmp_media.program_datetime = *localtime(&cur_time);
		//}
		tmp_media.buffers[tmp_media.nb_buffers] = buf;
		tmp_media.nb_buffers++;
		tmp_offset = 0;
	}

	memcpy(buf->data+tmp_offset, data, size);
	tmp_offset += size;
	tmp_media.content_length += size;
	return size;
}

static int RTStreamDone(AMessage *msg, int result)
{
	if (result != 0) {
		msg->done = NULL;
		return result;
	}

	if (tmp_avfx == NULL) {
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}

	AVStream *s = tmp_avfx->streams[vs_index];

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

		tmp_avpkt.pts = msh->time_sec*1000LL + msh->time_msec*10;
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

		STREAM_VIDEO_HEADER *vh = (STREAM_VIDEO_HEADER*)(sh + 1);
		if (sh->nFrameType == STREAM_FRAME_VIDEO_I) {
			tmp_avpkt.flags |= AV_PKT_FLAG_KEY;

			if (vh->nFrameRate != 0) {
				s->avg_frame_rate.num = vh->nFrameRate;
				av_stream_set_r_frame_rate(s, s->avg_frame_rate);
			}
		}

		if (sh->nHeaderSize >= sizeof(*sh)+sizeof(*vh)) {
			tmp_avpkt.pts = ((int64_t(vh->nTimeStampHight)<<32) + vh->nTimeStampLow)*1000
			              + vh->nTimeStampMillisecond;
		}
	}
	else {
		AMsgInit(msg, AMsgType_Unknown, NULL, 0);
		return 0;
	}

	tmp_avpkt.stream_index = vs_index;
	if (pts_offset == 0)
		pts_offset = tmp_avpkt.pts;
	tmp_avpkt.pts -= pts_offset;
	tmp_avpkt.pts = av_rescale(tmp_avpkt.pts, s->time_base.den, s->time_base.num*1000);

	bool push_buf = false;
	if ((s->cur_dts != AV_NOPTS_VALUE) && (s->cur_dts != 0))
	{
		if ((s->cur_dts >= tmp_avpkt.pts)
		 || (s->cur_dts+av_rescale(sec_per_file/2,s->time_base.den,s->time_base.num) < tmp_avpkt.pts))
		{
			int64_t diff = s->cur_dts - tmp_avpkt.pts;
			diff += av_rescale(s->avg_frame_rate.den, s->time_base.den, s->avg_frame_rate.num*s->time_base.num);

			TRACE("reset dts = %lld, pts = %lld, pts offset(%lld) - %lld.\n", s->cur_dts, tmp_avpkt.pts, pts_offset, diff);
			tmp_avpkt.pts += diff;
			pts_offset -= av_rescale(diff*1000, s->time_base.num, s->time_base.den);
		}
	}
	if ((s->first_dts == AV_NOPTS_VALUE) || (s->first_dts == 0)
	 || ((tmp_avpkt.flags & AV_PKT_FLAG_KEY)
	  && (s->first_dts+av_rescale(sec_per_file-1,s->time_base.den,s->time_base.num) < tmp_avpkt.pts)))
	{
		if ((s->first_dts != AV_NOPTS_VALUE) && (s->first_dts != 0) && (tmp_media.nb_buffers != 0)) {
			tmp_media.target_duration = av_rescale((tmp_avpkt.pts-s->first_dts)*1000, s->time_base.num, s->time_base.den);
		}
		TRACE("reset first dts(%lld => %lld), %d milliseconds.\n", s->first_dts, tmp_avpkt.pts, tmp_media.target_duration);
		s->first_dts = tmp_avpkt.pts;
		push_buf = true;
	}

	if (push_buf && (tmp_media.nb_buffers != 0)) {
		ARefsBuf *buf = tmp_media.buffers[tmp_media.nb_buffers-1];
		if (tmp_offset == 0) {
			ARefsBufRelease(buf);
			tmp_media.nb_buffers--;
		} else {
			buf->size = tmp_offset;
			tmp_offset = 0;
		}
		RTStreamPush();
	}

	int ret = av_write_frame(tmp_avfx, &tmp_avpkt);
	//TRACE("av_write_frame(%d) = %d, pts = %lld.\n", tmp_avpkt.size, ret, tmp_avpkt.pts);

	AMsgInit(msg, AMsgType_Unknown, NULL, 0);
	return 0;
}

static void av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	char log_buf[BUFSIZ];
	int len = vsprintf_s(log_buf, BUFSIZ-1, fmt, vl);
	log_buf[len] = '\0';
	OutputDebugStringA(log_buf);
	fputs(log_buf, stdout);
}

static void RTCheck(AOperator *opt, int result)
{
	if (result < 0) {
		if (rt != NULL)
			rt->cancel(rt, Aiosync_NotifyBack|0, &work_msg);
		if (rt_seq != 0) {
			pts_offset = 0;
			tmp_offset = 0;
			media_file_release(&tmp_media);

			for (int ix = 0; ix < _countof(rt_media); ++ix)
				media_file_release(&rt_media[ix]);
			pthread_mutex_destroy(&rt_mutex);
			rt_seq = 0;
		}
		release_s(tmp_avfx, avformat_close_output, NULL);
		opt->callback = NULL;
		return;
	}

	if ((rt != NULL) && (work_msg.done == NULL)) {
		AMsgInit(&work_msg, AMsgType_Unknown, NULL, 0);
		work_msg.done = &RTStreamDone;
		pts_offset = 0;

		result = rt->request(rt, Aiosync_NotifyBack|0, &work_msg);
		TRACE("rt stream register = %d.\n", result);
		if (result < 0)
			work_msg.done = NULL;
	}
	if (rt_seq == 0) {
		rt_seq = 1;
		pthread_mutex_init(&rt_mutex, NULL);
		memset(&rt_media, 0, sizeof(rt_media));

		memset(&tmp_media, 0, sizeof(tmp_media));
		tmp_offset = 0;
		pts_offset = 0;
	}
	if (tmp_avfx == NULL) {
		float a = 1.0f;
		av_log_set_level(AV_LOG_MAX_OFFSET);
		av_log_set_callback(&av_log_callback);
		av_register_all();

		mpegts_ofmt = av_guess_format("mpegts", NULL, NULL);
		h264_codec = avcodec_find_encoder(AV_CODEC_ID_H264);

		AVFormatContext *oc = NULL;
		int ret = avformat_open_output(&oc, mpegts_ofmt, NULL, &tmp_avio_write);
		if (ret >= 0)
			ret = avformat_new_output_stream(oc, h264_codec, NULL);

		if (ret >= 0) {
			vs_index = ret;
			ret = avformat_write_header(oc, NULL);
		}

		if (ret >= 0) {
			tmp_avfx = oc;
		} else {
			release_s(oc, avformat_close_output, NULL);
			TRACE("avformat create output(%s-%s) = %d.\n",
				mpegts_ofmt->name, h264_codec->name, ret);
		}
	}
	AOperatorTimewait(opt, NULL, 5*1000);
}

static int M3U8ProxyInit(AOption *option)
{
	AOption *opt = NULL;
	if (option != NULL)
		opt = AOptionFind(option, "m3u8_proxy");
	if ((opt == NULL) || (atol(opt->value) == 0))
		return 0;

	if (work_opt.callback != NULL)
		return 0;

	work_opt.callback = &RTCheck;
	AOperatorTimewait(&work_opt, NULL, 5*1000);
	return 1;
}

static void M3U8ProxyExit(void)
{
}

static int M3U8ProxyProbe(AObject *object, AMessage *msg)
{
	if (msg->type != AMsgType_Unknown)
		return -1;
	if ((strnicmp_c(msg->data, "GET /"live_m3u8" HTTP/") != 0)
	 && (strnicmp_c(msg->data, "GET /"live_prefix) != 0))
		return -1;
	return 80;
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
