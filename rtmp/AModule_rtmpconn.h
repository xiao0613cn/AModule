#ifndef _AMODULE_RTMPCONN_H_
#define _AMODULE_RTMPCONN_H_

#include "AModule_rtmp.h"

enum RtmpConnStatus {
	RCS_invalid = 0,

	RCS_opening,
	RCS_send_c0c1,
	RCS_recv_s0s1s2,
	RCS_send_c2,

	RCS_send_connect,
	RCS_send_outchunksize,
	RCS_send_releaseStream,
	RCS_send_FCPublish,
	RCS_send_createStream,
	RCS_send_publish,

	RCS_recv_result,
	RCS_opened,
};

struct RtmpConn {
	AObject    object;
	RTMPCtx    rtmp;
	AObject   *io;
	ARefsBuf  *buf;

	RtmpConnStatus status;
	AMessage   msg;
	AMessage  *from;
	RtmpConnStatus last_status;
	RTMPPacket pkt;
	RTMPPacket prev_pkt[5];
	ARefsBuf  *prev_buf[5];
};



#endif
