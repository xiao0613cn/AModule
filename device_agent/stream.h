#ifndef _AMODULE_STREAM_H_
#define _AMODULE_STREAM_H_

#include "../ecs/AEntity.h"
extern "C" {
#ifdef _WIN32
#pragma warning(disable: 4244) // 从“int”转换到“uint8_t”，可能丢失数据
#endif
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#ifdef _WIN32
#pragma warning(default: 4244)
#endif
};

// remark: struct AVPacket {
//   AVBufferRef *buf           => ARefsBuf *buf
//   int          stream_index  => enum AVMediaType stream_index
//   int64_t pts, dts, duration => AV_TIME_BASE per second(1/1000000)
// };

struct AEventManager;
struct AStreamInfo;
struct AStreamPlugin;
struct AStreamModule;

struct AStreamComponent : public AComponent {
	static const char *name() { return "AStreamComponent"; }
	AMODULE_GET(AStreamModule, AStreamComponent::name(), AStreamComponent::name())

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
	unsigned int      _key_flags;   // 1 << pkt->stream_index
	unsigned int      _media_flags; // 1 << pkt->stream_index
	unsigned int      _enable_key_ctrl : 1;

	int   (*on_recv)(AStreamPlugin *p, AVPacket *pkt);
	void   *on_recv_userdata;

	void init2() {
		_stream = NULL; _plugin_entry.init();
		on_recv = NULL; on_recv_userdata = NULL;
		_key_flags = ~(1u<<AVMEDIA_TYPE_VIDEO); // no video key frame
		_media_flags = -1u;                     // enable all media type
		_enable_key_ctrl = 1;
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
	AMODULE_GET(AStreamModule, AStreamComponent::name(), AStreamComponent::name())

	AStreamComponent* (*find)(AEntityManager *em, const char *stream_key);
	int   (*dispatch_avpkt)(AStreamComponent *c, AVPacket *pkt);
	int   (*sinfo_clone)(AStreamInfo **dest, AStreamInfo *src, int extra_bufsiz);
	void  (*sinfo_free)(AStreamInfo *info);
	void  (*avpkt_init)(AVPacket *pkt);
};

struct AStreamImplement {
	AModule module;
	// class_name: live, playback, download

	int   (*set_speed)(AEntity *e, void *req, void *resp);
	int   (*set_pos)(AEntity *e, void *req, void *resp);
	int   (*get_pos)(AEntity *e, void *req, void *resp);
	int   (*set_pause)(AEntity *e, void *req, void *resp);
	int   (*get_pause)(AEntity *e, void *req, void *resp);
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
