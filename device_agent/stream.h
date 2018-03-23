#ifndef _AMODULE_STREAM_H_
#define _AMODULE_STREAM_H_

#include "../ecs/AEntity.h"
extern "C" {
#ifdef _WIN32
#pragma warning(disable: 4244)
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#ifdef _WIN32
#pragma warning(default: 4244) // 从“int”转换到“uint8_t”，可能丢失数据
#endif
};

struct AEventManager;
struct AStreamInfo;
struct AStreamPlugin;

struct AStreamComponent : public AComponent {
	static const char *name() { return "AStreamComponent"; }

	char    _dev_id[48];
	int     _chan_id;
	int     _stream_id;
	char    _stream_key[64]; // live: <devid>_<chanid>_<stream_id>
	                         // playback: <devid>_<chanid>_<begin_time>_<random>
	                         // download: <devid>_<chanid>_<begin_time>_<end_time>
	AStreamInfo *_infos[AVMEDIA_TYPE_NB]; // AVMEDIA_TYPE_DATA => transcode aac...

	pthread_mutex_t  *_plugin_mutex;
	struct list_head  _plugin_list;
	int               _plugin_count;
	void    plugin_lock() { pthread_mutex_lock(_plugin_mutex); }
	void    plugin_unlock() { pthread_mutex_unlock(_plugin_mutex); }
	void    plugin_add(AStreamPlugin *p);
	void    plugin_del(AStreamPlugin *p);
	void    plugin_clear();

	int   (*on_recv)(AStreamComponent *s, AVPacket *pkt);
	void   *on_recv_userdata;
	// for live/playback/download, dispatch <pkt> to _plugin_list
	// for talk write to user object
	// pkt->stream_index: AVMEDIA_TYPE_XXX

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

struct AStreamInfo {
	AVCodecParameters param;
	int               extra_bufsiz;   
	unsigned int      has_key : 1;
	int64_t           last_pts;
};

struct AStreamPlugin : public AComponent {
	AStreamComponent *_stream;
	struct list_head  _plugin_entry;
	unsigned int      _key_flags;
	int   (*on_recv)(AStreamPlugin *p, AVPacket *pkt);

	void init2() {
		_stream = NULL; _plugin_entry.init(); on_recv = NULL;
		_key_flags = ~(1u<<AVMEDIA_TYPE_VIDEO); // no video key frame
	}
	static AStreamPlugin* first(list_head &list) {
		return list_first_entry(&list, AStreamPlugin, _plugin_entry);
	}
	AStreamPlugin* next() {
		return list_entry(_plugin_entry.next, AStreamPlugin, _plugin_entry);
	}
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
	int   (*sinfo_clone)(AStreamInfo **dest, AStreamInfo *src, int extra_bufsiz);
	void  (*sinfo_free)(AStreamInfo *info);
	void  (*avpkt_init)(AVPacket *pkt);
};

//////////////////////////////////////////////////////////////////////////
// inline function implement
inline void AStreamComponent::plugin_add(AStreamPlugin *p) {
	assert((p->_stream == NULL) && p->_plugin_entry.empty());
	p->_object->addref();
	p->_stream = this; this->_object->addref();

	_plugin_list.push_back(&p->_plugin_entry);
	++_plugin_count;
}

inline void AStreamComponent::plugin_del(AStreamPlugin *p) {
	assert((p->_stream == this) && !p->_plugin_entry.empty());
	p->_plugin_entry.leave();
	p->_object->release();
	--_plugin_count;
}

inline void AStreamComponent::plugin_clear() {
	while (!_plugin_list.empty()) {
		plugin_del(AStreamPlugin::first(_plugin_list));
	}
}

#endif
