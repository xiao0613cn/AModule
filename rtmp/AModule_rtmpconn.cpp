#include "stdafx.h"
#include "AModule_rtmpconn.h"
#include "../io/AModule_io.h"


static int RtmpConnCreate(AObject **object, AObject *parent, AOption *option)
{
	RtmpConn *rc = (RtmpConn *)*object;
	rtmp_init(&rc->rtmp, TRUE);
	rc->io = NULL;
	rc->buf = NULL;

	rc->status = RCS_invalid;

	AOption *io_option = AOptionFind(option, "io");
	if (io_option != NULL)
		AObjectCreate(&rc->io, &rc->object, io_option, NULL);
	return 1;
}

static void RtmpConnRelease(AObject *object)
{
	RtmpConn *rc = (RtmpConn *)object;
	release_f(rc->io, NULL, rc->io->release2());
	release_f(rc->buf, NULL, rc->buf->release2());
}

static int RtmpConnPushPkt(RtmpConn *rc, RTMPPacket *pkt)
{
	ARefsBufCheck(rc->buf, rtmp_guess_outsize(&rc->rtmp, pkt->size));

	int total_size = rtmp_gen_chunk_head(&rc->rtmp, rc->buf->next(), pkt, NULL);
	rc->buf->push(total_size);

	int off = 0;
	while (off < pkt->size) {
		int chunk_size = min(rc->rtmp.out_chunk_size, pkt->size-off);

		rc->buf->mempush(pkt->data+off, chunk_size);
		off += chunk_size;
		total_size += chunk_size;

		if (off < pkt->size) {
			chunk_size = rtmp_gen_next_head(&rc->rtmp, rc->buf->next(), pkt);
			rc->buf->push(chunk_size);
			total_size += chunk_size;
		}
	}
	return total_size;
}

static int RtmpConnRecvPkt(RtmpConn *rc, int result)
{
	rtmp_parse_one_chunk(&rc->rtmp, pkt, NULL);
}

int RtmpConnOpenStatus(RtmpConn *rc, int result)
{
	while (result > 0) {
		switch (rc->status)
		{
		case RCS_opening:
			ARefsBufCheck(rc->buf, 1+RTMP_HANDSHAKE_PACKET_SIZE*5, 0);
			rtmp_gen_c0c1(&rc->rtmp, rc->buf->next());

			rc->status = RCS_send_c0c1;
			rc->msg.init(ioMsgType_Block, rc->buf->next(), 1+RTMP_HANDSHAKE_PACKET_SIZE);
			result = ioInput(rc->io, &rc->msg);
			break;

		case RCS_send_c0c1:
			rc->status = RCS_recv_s0s1s2;
			rc->msg.init(ioMsgType_Block, rc->buf->next()+1+RTMP_HANDSHAKE_PACKET_SIZE, 1+RTMP_HANDSHAKE_PACKET_SIZE*2);
			result = ioOutput(rc->io, &rc->msg);
			break;

		case RCS_recv_s0s1s2:
			result = rtmp_gen_c2(&rc->rtmp, rc->buf->next(), rc->buf->next()+1+RTMP_HANDSHAKE_PACKET_SIZE,
				rc->buf->next()+1+RTMP_HANDSHAKE_PACKET_SIZE+1+RTMP_HANDSHAKE_PACKET_SIZE);
			if (result < 0)
				return result;

			rc->status = RCS_send_c2;
			rc->msg.init(ioMsgType_Block, rc->buf->next()+1, RTMP_HANDSHAKE_PACKET_SIZE);
			result = ioInput(rc->io, &rc->msg);
			break;

		case RCS_send_c2:
			rc->buf->reset();

			rtmp_gen_connect(&rc->rtmp, rc->buf->next(), &rc->pkt, NULL);
			rc->buf->push(rc->pkt.size);
			rc->buf->pop(rc->pkt.size);

			RtmpConnPushPkt(rc, &rc->pkt);
			rc->status = RCS_send_connect;
			rc->msg.init(ioMsgType_Block, rc->buf->ptr(), rc->buf->len());
			result = ioInput(rc->io, &rc->msg);
			break;

		case RCS_send_connect:
			rc->buf->pop(rc->buf->len());

			ARefsBufCheck(rc->buf, 32*1024, 0, NULL, NULL);
			rc->last_status = rc->status;
			rc->status = RCS_recv_result;
			result = 0;

		case RCS_recv_result:
			rc->buf->push(result);

			rc->pkt.data = rc->buf->ptr();
			rc->pkt.size = rc->buf->len();
			result = RtmpConnRecvPkt(rc, &rc->pkt);
			break;
		}
	}
}

static int RtmpConnOpen(AObject *object, AMessage *msg)
{
	RtmpConn *rc = (RtmpConn *)object;
	if ((rc->io == NULL) && (msg->type == AMsgType_Option))
	{
		AOption *io_option = AOptionFind((AOption*)msg->data, "io");
		if (io_option == NULL)
			return -EINVAL;

		int result = AObjectCreate(&rc->io, &rc->object, io_option, NULL);
		if (result < 0)
			return result;
	}

	rc->from = msg;
	rc->msg.init(msg);
	rc->msg.done = &TObjectDone(RtmpConn, msg, from, RtmpConnOpenStatus);

	rc->status = RCS_opening;
	int result = rc->io->open(&rc->msg);
	if (result != 0)
		result = rc->msg.done(&rc->msg, result);
	return result;
}

static int RtmpConnRequest(AObject *object, int reqix, AMessage *msg)
{

}
