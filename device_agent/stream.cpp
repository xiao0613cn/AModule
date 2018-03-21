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

static int strm_pkt_null(AStreamComponent *s, AVPacket *pkt)
{
	AModule *m = s->_object->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int strm_com_create(AObject **object, AObject *parent, AOption *options)
{
	AStreamComponent *s = (AStreamComponent*)*object;
	ADeviceComponent *dev = NULL;
	if ((parent != NULL) && (strcasecmp(parent->_module->class_name, "AEntity") == 0)) {
		((AEntity*)parent)->_get(&dev);
	}
	if (dev != NULL) {
		strcpy_sz(s->_dev_id, dev->_dev_id);
	} else {
		s->_dev_id[0] = '\0';
	}
	s->_chan_id = options->getInt("chan_id", 0);
	s->_stream_id = options->getInt("stream_id", 1);
	s->_stream_key[0] = '\0';

	memzero(s->_video_pars); memzero(s->_audio_pars);
	s->_video_pars.codec_type = s->_audio_pars.codec_type = AVMEDIA_TYPE_UNKNOWN;
	s->_video_pars.codec_id = s->_audio_pars.codec_id = AV_CODEC_ID_NONE;
	s->_video_pars.codec_tag = s->_audio_pars.codec_tag = 0;

	s->_plugin_mutex = NULL;
	s->_plugin_list.init();
	s->_plugin_count = 0;
	s->on_recv = SM.on_recv;
	s->send_to = &strm_pkt_null;

	s->_cur_speed = 1.0;
	s->set_speed = NULL;
	s->set_pos = NULL;
	s->get_pos = NULL;
	return 1;
}

static void strm_com_release(AObject *object)
{
	AStreamComponent *s = (AStreamComponent*)object;
	while (!s->_plugin_list.empty()) {
		AStreamPluginComponent *p = AStreamPluginComponent::first(s->_plugin_list);
		p->_plugin_entry.leave();

		p->_object->release();
	}
}

static int strm_com_recv_pkt(AStreamComponent *s, AVPacket *pkt)
{
	int count = 0;
	s->plugin_lock();

	AStreamPluginComponent *p = AStreamPluginComponent::first(s->_plugin_list);
	while (&p->_plugin_entry != &s->_plugin_list)
	{
		AStreamPluginComponent *next = p->next();

		int result = p->on_recv(p, pkt);
		if (result < 0) {
			p->_plugin_entry.leave();
			p->_object->release();
		} else {
			++count;
		}
		p = next;
	}
	s->plugin_unlock();
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
	&strm_com_recv_pkt,
	&strm_com_find,
	&avpkt_init,
};
static int reg_sm = AModuleRegister(&SM.module);
