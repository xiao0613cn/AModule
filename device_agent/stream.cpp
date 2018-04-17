#include "../stdafx.h"
#include "stream.h"
#include "device.h"
#include "frame_queue.h"

#ifdef _WIN32
#pragma comment(lib, "../win32/ffmpeg/lib/avcodec.lib")
#pragma comment(lib, "../win32/ffmpeg/lib/avformat.lib")
#pragma comment(lib, "../win32/ffmpeg/lib/avutil.lib")
#pragma comment(lib, "../win32/ffmpeg/lib/swresample.lib")
#endif

extern AStreamComponentModule SCM;

static void av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	/*char outbuf[BUFSIZ];
	int len = vsnprintf(outbuf, sizeof(outbuf)-1, fmt, vl);
	if (len < 0)
		len = BUFSIZ-1;
	outbuf[len] = '\0';
#ifdef _WIN32
	OutputDebugStringA(outbuf);
#endif
	if (level <= AV_LOG_INFO)
		fputs(outbuf, stdout);*/
	ALog(NULL, __FUNCTION__, level, fmt, vl);
}

static int SCM_init(AOption *global_option, AOption *module_option, BOOL first)
{
	if (first) {
#ifndef DISABLE_FFMPEG
		double a = 1.0;
		av_log_set_level(AV_LOG_MAX_OFFSET);
		av_log_set_callback(&av_log_callback);
		av_register_all();
		avformat_network_init();
		TRACE("%s.\n", avformat_configuration());

		/*h264_decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
		if (h264_decoder != NULL) {
			TRACE("find %s: %s.\n", h264_decoder->name, h264_decoder->long_name);
		}
		aac_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
		if (aac_encoder != NULL) {
			TRACE("find %s: %s.\n", aac_encoder->name, aac_encoder->long_name);
		}*/
#endif
	}
	return 1;
}

static void SCM_exit(int inited)
{
	if (inited > 0) {
	}
}

static int strm_com_create(AObject **object, AObject *parent, AOption *options)
{
	AStreamComponent *s = (AStreamComponent*)*object;
	ADeviceComponent *dev = NULL;
	if (parent != NULL) {
		((AEntity*)parent)->get(&dev);
	}
	if (dev != NULL) {
		strcpy_sz(s->_dev_id, dev->_dev_id);
	} else {
		s->_dev_id[0] = '\0';
	}
	s->_chan_id = options->getInt("chan_id", 0);
	s->_stream_id = options->getInt("stream_id", 1);
	s->_stream_key[0] = '\0';
	memzero(s->_infos);

	s->_plugin_mutex = NULL;
	s->_plugin_list.init();
	s->_plugin_count = 0;
	s->on_recv = SCM.dispatch_avpkt;
	s->on_recv_userdata = NULL;

	memzero(s->_begin_tm);
	memzero(s->_end_tm);
	s->_cur_speed = 1000;
	return 1;
}

static void strm_com_release(AObject *object)
{
	AStreamComponent *s = (AStreamComponent*)object;
	for (int ix = 0; ix < AVMEDIA_TYPE_NB; ++ix) {
		reset_s(s->_infos[ix], NULL, SCM.sinfo_free);
	}
	assert(s->_plugin_list.empty());
	assert(s->_plugin_count == 0);
}

static int strm_com_dispatch_mediainfo(AStreamComponent *s, AVPacket *pkt, AStreamPlugin *p)
{
	AVPacket inf_pkt;
	SCM.avpkt_init(&inf_pkt);

	inf_pkt.pts = pkt->pts;
	inf_pkt.stream_index = AVMEDIA_TYPE_NB;
	inf_pkt.data = (uint8_t*)s->_infos[pkt->stream_index];

	int result = p->on_recv(p, &inf_pkt);
	if (result < 0)
		s->plugin_del(p);
	else if (result > 0)
		p->_media_flags |= 1<<pkt->stream_index;
	return result;
}

static int strm_com_dispatch_avpkt(AStreamComponent *s, AVPacket *pkt)
{
	int count = 0;
	int mask = 1 << pkt->stream_index;

	AStreamPlugin *p = AStreamPlugin::first(s->_plugin_list);
	while (&p->_plugin_entry != &s->_plugin_list)
	{
		AStreamPlugin *next = p->next();
		if (!(p->_enable_flags & mask)) {
			p = next;
			continue;
		}
		if (!(p->_media_flags & mask)) {
			int result = strm_com_dispatch_mediainfo(s, pkt, p);
			if (result <= 0) {
				p = next;
				continue;
			}
		}
		if ((p->_enable_key_ctrls & mask) && !(p->_key_flags & mask)) {
			if (!(pkt->flags & AV_PKT_FLAG_KEY)) {
				p = next;
				continue;
			}
			p->_key_flags |= mask;
		}

		int result = p->on_recv(p, pkt);
		if (result < 0) {
			s->plugin_del(p);
		}
		else if (result == 0) {
			if (p->_enable_key_ctrls & mask) {
				p->_key_flags &= ~mask;
			}
		}
		else {
			++count;
		}
		p = next;
	}
	return count;
}

static AStreamComponent* strm_com_find(AEntityManager *em, const char *stream_key)
{
	AStreamComponent *s; em->upper_com(&s, NULL);
	while (s != NULL) {
		if (strcmp(s->_stream_key, stream_key) == 0)
			return s;
		s = em->next_com(s);
	}
	return NULL;
}

static int sinfo_clone(AStreamInfo **dest, AStreamInfo *src, int extra_bufsiz)
{
	AStreamInfo *p = *dest;
	if ((p != NULL) && (src != NULL) && (p->extra_bufsiz < src->param.extradata_size)) {
		SCM.sinfo_free(p);
		p = NULL;
	}
	if ((p != NULL) && (p->extra_bufsiz < extra_bufsiz)) {
		SCM.sinfo_free(p);
		p = NULL;
	}

	uint8_t *extra_data;
	if (p == NULL) {
		if ((src != NULL) && (extra_bufsiz < src->param.extradata_size + AV_INPUT_BUFFER_PADDING_SIZE)) {
			extra_bufsiz = src->param.extradata_size + AV_INPUT_BUFFER_PADDING_SIZE;
		}
		p = (AStreamInfo*)malloc(sizeof(*src) + extra_bufsiz);
		if (p == NULL) {
			*dest = NULL;
			return -ENOMEM;
		}
		extra_data = (uint8_t*)(p + 1);
	} else {
		extra_data = p->param.extradata;
		extra_bufsiz = p->extra_bufsiz;
	}

	if (src != NULL) {
		memcpy(p, src, sizeof(*src));
		memcpy(extra_data, src->param.extradata, src->param.extradata_size);
	} else {
		//codec_parameters_reset();
		p->param.codec_type          = AVMEDIA_TYPE_UNKNOWN;
		p->param.codec_id            = AV_CODEC_ID_NONE;
		p->param.format              = -1;
		p->param.field_order         = AV_FIELD_UNKNOWN;
		p->param.color_range         = AVCOL_RANGE_UNSPECIFIED;
		p->param.color_primaries     = AVCOL_PRI_UNSPECIFIED;
		p->param.color_trc           = AVCOL_TRC_UNSPECIFIED;
		p->param.color_space         = AVCOL_SPC_UNSPECIFIED;
		p->param.chroma_location     = AVCHROMA_LOC_UNSPECIFIED;
		p->param.sample_aspect_ratio.num = 0;
		p->param.sample_aspect_ratio.den = 1;
		p->param.profile             = FF_PROFILE_UNKNOWN;
		p->param.level               = FF_LEVEL_UNKNOWN;

		p->param.extradata_size = 0;
		p->last_pts = AV_NOPTS_VALUE;
	}
	p->param.extradata = extra_data;
	p->extra_bufsiz = extra_bufsiz;
	p->has_key = 0;
	*dest = p;
	return 1;
}

static void avpkt_init(AVPacket *pkt)
{
	memzero(*pkt);
	pkt->stream_index = AVMEDIA_TYPE_UNKNOWN;
	pkt->pts = pkt->dts = AV_NOPTS_VALUE;
	pkt->pos = -1;
}

static void avpkt_exit(AVPacket *pkt)
{
	reset_nif(pkt->buf, NULL, ((ARefsBuf*)pkt->buf)->release());

	if (pkt->stream_index == AVMEDIA_TYPE_NB) {
		reset_nif(pkt->data, NULL, SCM.sinfo_free((AStreamInfo*)pkt->data));
		pkt->stream_index = AVMEDIA_TYPE_UNKNOWN;
	}
}

static void avpkt_dup(AVPacket *pkt)
{
	if (pkt->buf != NULL) {
		((ARefsBuf*)pkt->buf)->addref();
	}
	if (pkt->stream_index == AVMEDIA_TYPE_NB) {
		AStreamInfo *si = (AStreamInfo*)pkt->data;

		if (SCM.sinfo_clone((AStreamInfo**)&pkt->data, si, 128) < 0) {
			assert(0); // ENOMEM
			pkt->data = NULL;
		}
	}
}

static int frame_queue_on_recv(AStreamPlugin *p, AVPacket *pkt)
{
	FrameQueueComponent *fq = (FrameQueueComponent*)p->on_recv_userdata;
	if (fq->_queue.size() >= fq->_queue._capacity()) {
		TRACE("%s: frame queue full(%d)...\n",
			p->_entity->_module->module_name, fq->_queue.size());
		return 0;
	}
	if ((fq->_queue.size() >= fq->_drop_ifnot_key) || !(pkt->flags & AV_PKT_FLAG_KEY)) {
		TRACE("%s: drop for not key frame, queue count = %d.\n",
			p->_entity->_module->module_name, fq->_queue.size());
		return 0;
	}

	int64_t pop_pts = fq->_pop_pts;
	if (pop_pts == AV_NOPTS_VALUE) {
		pop_pts = fq->_pop_pts = pkt->pts;
	}
	if ((pkt->pts > pop_pts) && (pkt->pts - pop_pts > fq->_max_delay)) {
		TRACE("%s: frame queue too late(%lld) to pop...\n",
			p->_entity->_module->module_name, pkt->pts - pop_pts);
		return 0;
	}

	if (fq->on_recv_hook != NULL) {
		p->on_recv_userdata = fq->on_recv_hook_userdata;
		int result = fq->on_recv_hook(p, pkt);
		p->on_recv_userdata = fq;
		if (result <= 0)
			return result;
	}
	SCM.avpkt_dup(pkt);
	fq->_put_pts = pkt->pts;
	fq->_queue.put_back(*pkt);
	return 1;
}

AStreamComponentModule SCM = { {
	AStreamComponent::name(),
	AStreamComponent::name(),
	sizeof(AStreamComponent),
	&SCM_init, &SCM_exit,
	&strm_com_create,
	&strm_com_release,
},
	&strm_com_find,
	&strm_com_dispatch_avpkt,
	&strm_com_dispatch_mediainfo,
	&sinfo_clone,
	(void (*)(AStreamInfo*))&free,
	&avpkt_init,
	&avpkt_exit,
	&avpkt_dup,
	&frame_queue_on_recv,
};
static int reg_scm = AModuleRegister(&SCM.module);


static int frame_queue_create(AObject **object, AObject *parent, AOption *option)
{
	FrameQueueComponent *fq = (FrameQueueComponent*)*object;
	fq->init2();
	fq->_max_delay = option->getI64("max_delay", fq->_max_delay/AV_TIME_BASE)*AV_TIME_BASE;
	fq->_drop_ifnot_key = option->getInt("drop_ifnot_key", fq->_drop_ifnot_key);

	/*AStreamPlugin *p = NULL;
	if (parent != NULL)
		((AEntity*)parent)->get(&p);
	if (p != NULL) {
		fq->on_recv_hook = p->on_recv;
		fq->on_recv_hook_userdata = p->on_recv_userdata;
		p->on_recv = frame_queue_on_recv;
		p->on_recv_userdata = fq;
	}*/
	return 1;
}

AModule FQM = {
	FrameQueueComponent::name(),
	FrameQueueComponent::name(),
	sizeof(FrameQueueComponent),
	NULL, NULL,
	&frame_queue_create,
};
static int reg_fqm = AModuleRegister(&FQM);
