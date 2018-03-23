#ifndef 
#define 
#include "stream.h"


struct TranscodeModule {
	AModule module;
	int (*open)(AComponent *tx, AVCodecParameters *dest, AVCodecParameters *src);
	int (*get_dest_sinfo)(AComponent *tx, AStreamInfo **sinfo);
	int (*transcode)(AComponent *tx, AVPacket *dest, AVPacket *src);
	int (*close)(AComponent *tx);
};

struct FFmpegAudioTranscode : public AComponent {
	static const char* name() { return "FFmpegAudioTranscode"; }
	static TranscodeModule* Module() {
		static TranscodeModule* s_m = (TranscodeModule*)
			AModuleFind(name(), name());
		return s_m;
	}

	int   (*on_recv_hook)(AStreamComponent *s, AVPacket *pkt);
	void   *on_recv_hookdata;

	AVCodecContext *_araw_ctx;
	AVFrame         _araw_frame;
	ARefsBuf       *_araw_buf[AV_NUM_DATA_POINTERS];
	int             _araw_linesize;
	int             _araw_planar_count;

	AVCodecContext *_aac_ctx;
	int             _aac_linesize;
	int             _aac_planar_count;
	ARefsBuf       *_aac_buf;
	struct SwrContext *_swr_ctx;
	uint8_t           *_swr_buf[AV_NUM_DATA_POINTERS];

};

#endif
