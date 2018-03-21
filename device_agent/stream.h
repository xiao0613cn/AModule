#ifndef _AMODULE_STREAM_H_
#define _AMODULE_STREAM_H_

#include "../ecs/AEntity.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

#ifndef feq
#define feq(a, b)  (fabs(a-b) < 0.000001)
#endif

struct AStreamPluginComponent;
struct AEventManager;

struct AStreamComponent : public AComponent {
	static const char *name() { return "AStreamComponent"; }

	char    _dev_id[48];
	int     _chan_id;
	int     _stream_id;
	char    _stream_key[64]; // live: <devid>_<chanid>_<stream_id>
	                         // playback: <devid>_<chanid>_<begin_time>_<random>
	                         // download: <devid>_<chanid>_<begin_time>_<end_time>
	AVCodecParameters _video_pars;
	AVCodecParameters _audio_pars;

	pthread_mutex_t  *_plugin_mutex;
	struct list_head  _plugin_list;
	int               _plugin_count;
	void    plugin_lock() { pthread_mutex_lock(_plugin_mutex); }
	void    plugin_unlock() { pthread_mutex_unlock(_plugin_mutex); }
	void    plugin_add(AStreamPluginComponent *p);

	// for live/playback/download, dispatch <pkt> to _plugin_list
	int   (*on_recv)(AStreamComponent *s, AVPacket *pkt);

	// for talk
	int   (*send_to)(AStreamComponent *s, AVPacket *pkt);

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


struct AStreamPluginComponent : public AComponent {
	static const char* name() { return "AStreamPluginComponent"; }

	char    _dev_id[48];
	int     _chan_id;
	int     _stream_id;
	char    _stream_key[64];

	struct list_head _plugin_entry;
	static AStreamPluginComponent* first(list_head &list) {
		return list_first_entry(&list, AStreamPluginComponent, _plugin_entry);
	}
	AStreamPluginComponent* next() {
		return list_entry(_plugin_entry.next, AStreamPluginComponent, _plugin_entry);
	}

	int (*on_recv)(AStreamPluginComponent *s, AVPacket *pkt);
};



struct AStreamModule {
	AModule module;
	static AStreamModule* get() {
		static AStreamModule *s_m = (AStreamModule*)AModuleFind(
			AStreamComponent::name(), AStreamComponent::name());
		return s_m;
	}

	int  (*on_recv)(AStreamComponent *c, AVPacket *pkt);
	AStreamComponent* (*find)(AEntityManager *em, const char *stream_key);

	// util
	void (*avpkt_init)(AVPacket *pkt);
};


#endif
