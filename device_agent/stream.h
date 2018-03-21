#ifndef _AMODULE_STREAM_H_
#define _AMODULE_STREAM_H_

#include "../ecs/AEntity.h"
struct AEventManager;

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

#ifndef feq
#define feq(a, b)  (fabs(a-b) < 0.000001)
#endif


struct AStreamComponent : public AComponent {
	static const char *name() { return "AStreamComponent"; }

	char    _dev_id[48];
	int     _chan_id;
	int     _stream_id;
	char    _stream_key[64]; // live: <devid>_<chanid>_<stream_id>
	                         // playback: <devid>_<chanid>_<begin_time>_<random>
	                         // download: <devid>_<chanid>_<begin_time>_<end_time>
	AVCodecParameters _video_pars; // AVPacket.stream_index = 0
	AVCodecParameters _audio_pars; // AVPacket.stream_index = 1
	int&    video_fps()          { return _video_pars.sample_rate; }
	BOOL&   video_has_keyframe() { return _video_pars.initial_padding; }
	int     _video_extra_bufsiz;
	int     _audio_extra_bufsiz;

	pthread_mutex_t  *_plugin_mutex;
	struct list_head  _plugin_list;
	int               _plugin_count;
	void    plugin_lock() { pthread_mutex_lock(_plugin_mutex); }
	void    plugin_unlock() { pthread_mutex_unlock(_plugin_mutex); }
	void    plugin_add(AStreamComponent *p);
	bool    is_plugin();
	AStreamComponent* plugin_first();
	AStreamComponent* plugin_next();

	// for live/playback/download, dispatch <pkt> to _plugin_list
	// for talk write to user object
	int   (*on_recv)(AStreamComponent *s, AVPacket *pkt);

	/* for playback | download */
	struct tm _begin_tm;
	struct tm _end_tm;
	float     _cur_speed;
	int   (*set_speed)(AStreamComponent *s, AOption *opts);
	int   (*set_pos)(AStreamComponent *s, AOption *opts);
	int   (*get_pos)(AStreamComponent *s, AOption *opts);
	//int (*set_pause)(struct TopicProc *proc);
	//int (*get_pause)(struct TopicProc *proc);
	/* for playback | download */

	//AEventManager *_plugin_manager;
	// "on_stream_open", AStreamComponent *s
	// "on_stream_recv_packet", AVPacket *pkt
	// "on_stream_close", AStreamComponent *s
};

struct AStreamModule {
	AModule module;
	static AStreamModule* get() {
		static AStreamModule *s_m = (AStreamModule*)AModuleFind(
			AStreamComponent::name(), AStreamComponent::name());
		return s_m;
	}

	AStreamComponent* (*find)(AEntityManager *em, const char *stream_key);
	int   (*dispatch_avpkt)(AStreamComponent *c, AVPacket *pkt);
	int   (*save_pars)(AStreamComponent *s, AVCodecParameters *pars);
	void  (*avpkt_init)(AVPacket *pkt);
};

//////////////////////////////////////////////////////////////////////////
// inline function implement
inline void AStreamComponent::plugin_add(AStreamComponent *p) {
	_plugin_list.push_back(&p->_plugin_list);
	++_plugin_count;
}
inline bool AStreamComponent::is_plugin() {
	return (!_plugin_list.empty() && (_plugin_count == 0));
}
inline AStreamComponent* AStreamComponent::plugin_first() {
	return list_first_entry(&_plugin_list, AStreamComponent, _plugin_list);
}
inline AStreamComponent* AStreamComponent::plugin_next() {
	assert(is_plugin());
	return list_entry(_plugin_list.next, AStreamComponent, _plugin_list);
}

#endif
