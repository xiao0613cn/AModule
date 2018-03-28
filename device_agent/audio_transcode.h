#ifndef _AUDIO_TRANSCODE_H_
#define _AUDIO_TRANSCODE_H_
#include "stream.h"


struct TranscodeModule {
	AModule   module;
	int     (*open)(AComponent *c, AStreamInfo **pinfo, AVCodecParameters *src);
	int     (*transcode)(AComponent *c, AVPacket *dest, AVPacket *src);
	void      close(AComponent *c) { module.release((AObject*)c); }
	int64_t (*reset_pts)(AComponent *c, int64_t pts/*=AV_NOPTS_VALUE*/);
};

struct FFmpegAudioTranscode : public AComponent {
	static const char* name() { return "FFmpegAudioTranscode"; }
	AMODULE_GET(TranscodeModule, name(), name())

	//int   (*on_recv_hook)(AStreamComponent *s, AVPacket *pkt);
	//void   *on_recv_hookdata;

	AVCodecContext *_araw_ctx;
	AVFrame         _araw_frame;
	ARefsBuf       *_araw_buf[AV_NUM_DATA_POINTERS];
	int             _araw_linesize;
	int             _araw_planar_count;

	AVCodecContext *_aac_ctx;
	int             _aac_linesize;
	int             _aac_planar_count;
	ARefsBuf       *_aac_buf;
	int64_t         _aac_pts;
	struct SwrContext *_swr_ctx;
	uint8_t           *_swr_buf[AV_NUM_DATA_POINTERS];
};

#endif
