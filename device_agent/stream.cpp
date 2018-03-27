#include "../stdafx.h"
#include "stream.h"
#include "device.h"

#ifdef _WIN32
#pragma comment(lib, "../win32/ffmpeg/lib/avcodec.lib")
#pragma comment(lib, "../win32/ffmpeg/lib/avformat.lib")
#pragma comment(lib, "../win32/ffmpeg/lib/avutil.lib")
#pragma comment(lib, "../win32/ffmpeg/lib/swresample.lib")
#endif

extern AStreamModule SM;

static void av_log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	char outbuf[BUFSIZ];
	int len = vsnprintf(outbuf, sizeof(outbuf)-1, fmt, vl);
	if (len < 0)
		len = BUFSIZ-1;
	outbuf[len] = '\0';
#ifdef _WIN32
	OutputDebugStringA(outbuf);
#endif
	if (level <= AV_LOG_INFO)
		fputs(outbuf, stdout);
}

static int SM_init(AOption *global_option, AOption *module_option, BOOL first)
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

static void SM_exit(int inited)
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
	s->_chan_id = 0;
	s->_stream_id = 1;
	s->_stream_key[0] = '\0';
	memzero(s->_infos);

	s->_plugin_mutex = NULL;
	s->_plugin_list.init();
	s->_plugin_count = 0;
	s->on_recv = SM.dispatch_avpkt;
	s->on_recv_userdata = NULL;

	memzero(s->_begin_tm);
	memzero(s->_end_tm);
	s->_cur_speed = 1.0;
	return 1;
}

static void strm_com_release(AObject *object)
{
	AStreamComponent *s = (AStreamComponent*)object;
	for (int ix = 0; ix < AVMEDIA_TYPE_NB; ++ix) {
		reset_s(s->_infos[ix], NULL, SM.sinfo_free);
	}
	assert(s->_plugin_list.empty());
	assert(s->_plugin_count == 0);
}

static int strm_com_dispatch_avpkt(AStreamComponent *s, AVPacket *pkt)
{
	int count = 0;
	AStreamPlugin *p = AStreamPlugin::first(s->_plugin_list);
	while (&p->_plugin_entry != &s->_plugin_list)
	{
		AStreamPlugin *next = p->next();
		if (p->_enable_key_ctrl && !(p->_key_flags & (1<<pkt->stream_index))) {
			if (!(pkt->flags & AV_PKT_FLAG_KEY)) {
				p = next;
				continue;
			}
			p->_key_flags |= (1<<pkt->stream_index);
		}

		int result = p->on_recv(p, pkt);
		if (result < 0) {
			s->plugin_del(p);
		}
		else if (result == 0) {
			if (p->_enable_key_ctrl) {
				p->_key_flags &= ~(1<<pkt->stream_index);
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
		SM.sinfo_free(p);
		p = NULL;
	}
	if ((p != NULL) && (p->extra_bufsiz < extra_bufsiz)) {
		SM.sinfo_free(p);
		p = NULL;
	}

	uint8_t *extra_data;
	if (p == NULL) {
		if ((src != NULL) && (extra_bufsiz < src->param.extradata_size)) {
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
	pkt->pts = pkt->dts = AV_NOPTS_VALUE;
	pkt->pos = -1;
}

AStreamModule SM = { {
	AStreamComponent::name(),
	AStreamComponent::name(),
	sizeof(AStreamComponent),
	&SM_init, &SM_exit,
	&strm_com_create,
	&strm_com_release,
},
	&strm_com_find,
	&strm_com_dispatch_avpkt,
	&sinfo_clone,
	(void (*)(AStreamInfo*))&free,
	&avpkt_init,
};
static int reg_sm = AModuleRegister(&SM.module);
