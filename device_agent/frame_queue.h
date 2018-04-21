#ifndef _AMODULE_FRAME_QUEUE_H_
#define _AMODULE_FRAME_QUEUE_H_

#include "stream.h"
#include "../base/srsw.hpp"


struct FrameQueueComponent : public AComponent {
	static const char* name() { return "FrameQueueComponent"; }
	AMODULE_GET(AModule, name(), name());

	int   (*on_recv_hook)(AStreamPlugin *p, AVPacket *pkt);
	void   *on_recv_hook_userdata;

	srsw_queue<AVPacket, 32> _queue;
	int64_t _put_pts;
	int64_t _pop_pts;

	int64_t _max_delay;
	unsigned _drop_ifnot_key; // max video frame queue count

	void init2() {
		on_recv_hook = NULL; on_recv_hook_userdata = NULL;
		_queue.reset();
		_put_pts = _pop_pts = AV_NOPTS_VALUE;
		_max_delay = 2*AV_TIME_BASE; // drop if (pkt->pts - _pop_pts > _max_delay);
		_drop_ifnot_key = 6;  // drop if (_queue.size() >= _drop_ifnot_key || !(pkt->flags & AV_PKT_FLAG_KEY));
	}
	void exit2() {
		while (_queue.size() != 0) {
			AStreamComponentModule::get()->avpkt_exit(&_queue.front());
			_queue.pop_front();
		}
	}
	int64_t delay() {
		int64_t put_pts = _put_pts, pop_pts = _pop_pts;
		if ((put_pts == AV_NOPTS_VALUE) || (pop_pts == AV_NOPTS_VALUE) || (put_pts < pop_pts))
			return 0;
		return (put_pts - pop_pts);
	}
};


#endif
