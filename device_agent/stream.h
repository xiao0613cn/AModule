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

#define AV_PKT_FLAG_CODECPAR   0x0008 // AVCodecParameters has changed
#ifdef _WIN32
#pragma warning(default: 4244)
#endif
};

// remark: struct AVPacket {
//   AVBufferRef *buf           => ARefsBuf *buf
//   int          stream_index  => enum AVMediaType stream_index, +AVMEDIA_TYPE_NB: data = AStreamInfo*
//   int64_t pts, dts, duration => AV_TIME_BASE per second(1/1000000)
// };

struct AEventManager;
struct AStreamInfo;
struct AStreamPlugin;

struct AStreamComponent : public AComponent {
	static const char *name() { return "AStreamComponent"; }
	AMODULE_GET(struct AStreamComponentModule, AStreamComponent::name(), AStreamComponent::name())

	char    _dev_id[48];
	int     _chan_id;
	int     _stream_id;
	AStreamInfo *_infos[AVMEDIA_TYPE_NB]; // AVMEDIA_TYPE_DATA => transcode aac...

	pthread_mutex_t  *_plugin_mutex;
	struct list_head  _plugin_list;
	int               _plugin_count;
	void    plugin_lock() { pthread_mutex_lock(_plugin_mutex); }
	void    plugin_unlock() { pthread_mutex_unlock(_plugin_mutex); }
	void    plugin_add(AStreamPlugin *p);
	void    plugin_del(AStreamPlugin *p);
	void    plugin_clear();

	int   (*do_recv)(AStreamComponent *s);
	// do_recv == NULL: auto start for on_recv() cycle callback

	int   (*on_recv)(AStreamComponent *s, AVPacket *pkt, int result);
	void   *on_recv_userdata;
	// on_recv() >= AMsgType_Class: pending recv cycle, must call do_recv() for next time
	// on_recv() > 0: continue recv cycle, will callback on_recv() when new packet
	// on_recv() == 0: panding internal io operation
	// on_recv() < 0:  end recv cycle
	// for live/playback/download, dispatch <pkt> to _plugin_list
	// for talk write to user object

	/* for playback | download */
	struct tm _begin_tm;
	struct tm _end_tm;
	int       _cur_speed; // default 1000, unit: (1/1000) permillage
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
	static const char *name() { return "AStreamPlugin"; }

	AStreamComponent *_stream;
	struct list_head  _plugin_entry;
	int   (*on_recv)(AStreamPlugin *p, AVPacket *pkt);
	void   *on_recv_userdata;

	// all flags: 1 << pkt->stream_index
	unsigned int      _enable_flags;
	unsigned int      _media_flags;
	unsigned int      _key_flags;
	unsigned int      _enable_key_ctrls;

	void init2() {
		_stream = NULL; _plugin_entry.init();
		on_recv = NULL; on_recv_userdata = NULL;

		_enable_flags = unsigned(-1);                    // enable all media type
		_media_flags = 0;
		_key_flags = ~(1u<<AVMEDIA_TYPE_VIDEO); // no video key frame
		_enable_key_ctrls = unsigned(-1);
	}
	static AStreamPlugin* first(list_head &list) {
		return list_first_entry(&list, AStreamPlugin, _plugin_entry);
	}
	AStreamPlugin* next() {
		return list_entry(_plugin_entry.next, AStreamPlugin, _plugin_entry);
	}
};

struct AStreamComponentModule {
	AModule module;
	AMODULE_GET(AStreamComponentModule, AStreamComponent::name(), AStreamComponent::name())

	int   (*dispatch_avpkt)(AStreamComponent *s, AVPacket *pkt, int result);
	int   (*dispatch_mediainfo)(AStreamComponent *s, AVPacket *pkt, AStreamPlugin *p);
	int   (*peek_h264_spspps)(AStreamComponent *s, AVPacket *pkt);
	int   (*sinfo_clone)(AStreamInfo **dest, AStreamInfo *src, int extra_bufsiz);
	void  (*sinfo_free)(AStreamInfo *info);
	void  (*avpkt_init)(AVPacket *pkt);
	void  (*avpkt_exit)(AVPacket *pkt);
	void  (*avpkt_dup)(AVPacket *pkt);
	int   (*frame_queue_on_recv)(AStreamPlugin *p, AVPacket *pkt);
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
	p->_entity->addref();
	p->_stream = this; this->_entity->addref();

	_plugin_list.push_back(&p->_plugin_entry);
	++_plugin_count;
}

inline void AStreamComponent::plugin_del(AStreamPlugin *p) {
	assert((p->_stream == this) && !p->_plugin_entry.empty());
	p->_plugin_entry.leave();
	p->_entity->release();
	--_plugin_count;
}

inline void AStreamComponent::plugin_clear() {
	while (!_plugin_list.empty()) {
		plugin_del(AStreamPlugin::first(_plugin_list));
	}
}

#endif
