#include "../stdafx.h"
#include "../ecs/AClientSystem.h"
#include "../ecs/AInOutComponent.h"
#include "../device_agent/stream.h"
#include "../device_agent/device.h"
#include "PvdNetCmd.h"
#include "../media/h264_decode_sps.h"


struct PVDStream : public AEntity {
	AClientComponent _client;
	AStreamComponent _stream;
	IOObject   *_io;
	AOption    *_io_opt;
	ARefsBuf   *_buffer;

	PVDStatus _status;
	HBNetCmdComponent *_netcmd;
	AMessage    _heart_msg;
	int open(int result);
};

static inline void tm_to_MFS_FIELD_TIME(struct tm *tm, MFS_FIELD_TIME *ut)
{
	ut->nYear   = tm->tm_year + 1900 - 2000;
	ut->uMonth  = tm->tm_mon + 1;
	ut->uDay    = tm->tm_mday;
	ut->uHour   = tm->tm_hour;
	ut->uMinute = tm->tm_min;
	ut->uSecond = tm->tm_sec;
}

static int PVDTryPeekFrame(uint32_t userid, ARefsBuf *outbuf, AMessage &outmsg)
{
#define TAG_SIZE  4
#define MAKE_TAG(ptr)  (uint8_t(ptr[0])|(uint8_t(ptr[1])<<8)|(uint8_t(ptr[2])<<16)|(uint8_t(ptr[3])<<24))

	//outmsg.init();
	int result = outbuf->len();
	if (result < TAG_SIZE)
		return 0;

	int tag = MAKE_TAG(outbuf->ptr());
	if (tag == MSHDV2_FLAG) {
		if (result < sizeof(MSHEAD))
			return 0;
		result = MSHEAD_GETFRAMESIZE(outbuf->ptr());
	}
	else if (tag == MSHEAD_FLAG) {
		if (result < sizeof(MSHEAD))
			return 0;
		result = MSHEAD_GETFRAMESIZE(outbuf->ptr());
	}
	else if (tag == STREAM_HEADER_FLAG) {
		if (result < sizeof(STREAM_HEADER))
			return 0;
		STREAM_HEADER *sh = (STREAM_HEADER*)outbuf->ptr();
		result = sh->nHeaderSize + sh->nEncodeDataSize;
	}
	else if (tag == NET_CMD_HEAD_FLAG) {
		result = PVDCmdDecode(userid, outbuf->ptr(), result);
		if (result <= 0) {
			TRACE("PVDCmdDecode(%d, %d) = %d.\n", userid, outbuf->len(), result);
			return result;
		}
	}
	else {
		TRACE2("unsupport format: 0x%X, size = %d.\n", tag, result);
		outbuf->pop(1);
		return -EAGAIN;
	}

	outmsg.init(tag, outbuf->ptr(), result);
	if (result > outbuf->len()) {
		//if (ioMsgType_isBlock(outmsg.type)) {
		//	TRACE("reset buffer(%d), drop data(%X).\n", outmsg.size, tag);
		//	outbuf->pop(outmsg.size);
		//}
		return 0;
	}
	outbuf->pop(result);
	return 1;
}

static int msh_pkt(PVDStream *s, AVPacket &pkt)
{
	MSHEAD *msh = (MSHEAD*)s->_heart_msg.data;
	pkt.pts = msh->time_sec*AV_TIME_BASE + msh->time_msec*AV_TIME_BASE/1000;
	pkt.data = (uint8_t*)MSHEAD_DATAP(msh);
	pkt.size = MSHEAD_GETMSDSIZE(msh);

	pkt.stream_index = ISVIDEOFRAME(msh) ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
	if (ISKEYFRAME(msh))
		pkt.flags |= AV_PKT_FLAG_KEY;
	return 1;
}
static void si_codec(AStreamInfo *si, AVPacket &pkt, enum AVCodecID codec_id)
{
	if (codec_id != si->param.codec_id) {
		pkt.flags |= AV_PKT_FLAG_CODECPAR;

		si->param.codec_id = codec_id;
		si->param.codec_type = (AVMediaType)pkt.stream_index;
		if (pkt.stream_index == AVMEDIA_TYPE_VIDEO) {
			si->param.format = AV_PIX_FMT_YUV420P;
		} else {
			si->param.format = AV_SAMPLE_FMT_S16;
			si->param.sample_rate = 8000;
			si->param.channels = 1;
		}
	}
}
static void si_video(AStreamInfo *si, AVPacket &pkt, int width, int height)
{
	if (si->param.width != width) {
		pkt.flags |= AV_PKT_FLAG_CODECPAR;
		si->param.width = width;
	}
	if (si->param.height != height) {
		pkt.flags |= AV_PKT_FLAG_CODECPAR;
		si->param.height = height;
	}
}
static int msh_info(PVDStream *s, AVPacket &pkt)
{
	AStreamInfo *si = s->_stream._infos[pkt.stream_index];
	enum AVCodecID codec_id;

	MSHEAD *msh = (MSHEAD*)s->_heart_msg.data;
	switch (msh->algorithm)
	{
	case ISO_G722:  codec_id = AV_CODEC_ID_ADPCM_G722; break;
	//case ISO_G728:  break;
	case ISO_G729:  codec_id = AV_CODEC_ID_G729; break;
	case ISO_PCM:   codec_id = AV_CODEC_ID_PCM_S16LE; break;
	case ISO_G711A: codec_id = AV_CODEC_ID_PCM_ALAW; break;
	case ISO_G726:  codec_id = AV_CODEC_ID_ADPCM_G726; break;

	case ISO_MPEG4: codec_id = AV_CODEC_ID_MPEG4; break;
	case ISO_H264: case ISO_H264_2X: case ISO_H264_4X:
	{
		int result = AStreamComponent::get()->peek_h264_spspps(&s->_stream, &pkt);
		if (result < 0)
			return result;
		if (result > 0) {
			pkt.flags |= AV_PKT_FLAG_KEY;
			si = s->_stream._infos[pkt.stream_index];
		}
		codec_id = AV_CODEC_ID_H264;
		break;
	}
	case ISO_PIC_JPG: codec_id = AV_CODEC_ID_JPEG2000; break;
	default:
		TRACE("PVDStream(%s, %d, %d): unknown algorithm: %d.\n",
			s->_stream._dev_id, s->_stream._chan_id, s->_stream._stream_id, msh->algorithm);
		return -EINVAL;
	}

	si_codec(si, pkt, codec_id);
	if (pkt.stream_index == AVMEDIA_TYPE_VIDEO) {
		si_video(si, pkt, msh->width*16, msh->height*16);
	}
	return 1;
}
static int shv3_pkt(PVDStream *s, AVPacket &pkt)
{
	STREAM_HEADER *sh = (STREAM_HEADER*)s->_heart_msg.data;
	pkt.data = (uint8_t*)sh + sh->nHeaderSize;
	pkt.size = sh->nEncodeDataSize;

	pkt.stream_index = (sh->nFrameType == STREAM_FRAME_AUDIO) ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;
	if (pkt.stream_index == AVMEDIA_TYPE_VIDEO) {
		if (sh->nFrameType == STREAM_FRAME_VIDEO_I)
			pkt.flags |= AV_PKT_FLAG_KEY;

		STREAM_VIDEO_HEADER *vh = (STREAM_VIDEO_HEADER*)(sh + 1);
		pkt.pts = vh->nTimeStampLow*AV_TIME_BASE + vh->nTimeStampMillisecond*AV_TIME_BASE/1000;
	}
	return 1;
}
static int shv3_info(PVDStream *s, AVPacket &pkt)
{
	AStreamInfo *si = s->_stream._infos[pkt.stream_index];
	enum AVCodecID codec_id;

	STREAM_HEADER *sh = (STREAM_HEADER*)s->_heart_msg.data;
	if (pkt.stream_index == AVMEDIA_TYPE_VIDEO) {
		switch (sh->nAlgorithm)
		{
		case STREAM_ALGORITHM_VIDEO_H264_HISILICON:
		case STREAM_ALGORITHM_VIDEO_H264_AMBARELLA:
		case STREAM_ALGORITHM_VIDEO_H264_TECHWELL:
		case STREAM_ALGORITHM_VIDEO_H264_GENERAL:
		{
			int result = AStreamComponent::get()->peek_h264_spspps(&s->_stream, &pkt);
			if (result < 0)
				return result;
			if (result > 0) {
				pkt.flags |= AV_PKT_FLAG_KEY;
				si = s->_stream._infos[pkt.stream_index];
			}
			codec_id = AV_CODEC_ID_H264;
			break;
		}
		case STREAM_ALGORITHM_VIDEO_H265_HISILICON:
		case STREAM_ALGORITHM_VIDEO_H265_GENERAL:
			codec_id = AV_CODEC_ID_H265;
			break;
		case STREAM_ALGORITHM_VIDEO_JPEG:
			codec_id = AV_CODEC_ID_JPEG2000;
			break;
		case STREAM_ALGORITHM_VIDEO_MPEG4:
		case STREAM_ALGORITHM_VIDEO_MPEG4_ISO:
			codec_id = AV_CODEC_ID_MPEG4;
			break;
		default: return -EINVAL;
		}
		STREAM_VIDEO_HEADER *vh = (STREAM_VIDEO_HEADER*)(sh + 1);
		si_video(si, pkt, vh->nWidth*4, vh->nHeight*4);
	} else {
		switch (sh->nAlgorithm)
		{
		case STREAM_ALGORITHM_AUDIO_PCM8_16: codec_id = AV_CODEC_ID_PCM_U8; break;
		case STREAM_ALGORITHM_AUDIO_G711A:   codec_id = AV_CODEC_ID_PCM_ALAW; break;
		case STREAM_ALGORITHM_AUDIO_G722:    codec_id = AV_CODEC_ID_ADPCM_G722; break;
		case STREAM_ALGORITHM_AUDIO_G726:    codec_id = AV_CODEC_ID_ADPCM_G726; break;
		case STREAM_ALGORITHM_AUDIO_PCM16:   codec_id = AV_CODEC_ID_PCM_S16LE; break;
		case STREAM_ALGORITHM_AUDIO_G711U:   codec_id = AV_CODEC_ID_PCM_MULAW; break;
		default: return -EINVAL;
		}
	}
	si_codec(si, pkt, codec_id);
	return 1;
}
static struct {
	int    type_tag;
	int  (*avpkt_set)(PVDStream *s, AVPacket &pkt);
	int  (*sinfo_set)(PVDStream *s, AVPacket &pkt);
} FrameOps[] = {
	{ MSHDV2_FLAG, msh_pkt, msh_info },
	{ MSHEAD_FLAG, msh_pkt, msh_info },
	{ STREAM_HEADER_FLAG, shv3_pkt, shv3_info },
	{ 0, NULL, NULL }
};

static int PVDStreamDispatch(PVDStream *s)
{
	AVPacket pkt;
	AStreamComponentModule *SCM = AStreamComponent::get();
	SCM->avpkt_init(&pkt);

	for (int ix = 0; FrameOps[ix].type_tag != 0; ++ix) {
		if (s->_heart_msg.type == FrameOps[ix].type_tag)
		{
			int result = FrameOps[ix].avpkt_set(s, pkt);
			pkt.buf = (AVBufferRef*)s->_buffer;
			if (result >= 0) {
				if (s->_stream._infos[pkt.stream_index] == NULL)
					SCM->sinfo_clone(&s->_stream._infos[pkt.stream_index], NULL, 128);

				result = FrameOps[ix].sinfo_set(s, pkt);
			}
			result = s->_stream.on_recv(&s->_stream, &pkt, result);
			return result;
		}
	}
	// ignore unknown packet
	return 1;
}

static int PVDStreamOutputDone(AMessage *msg, int result)
{
	PVDStream *s = container_of(msg, PVDStream, _heart_msg);
	for (;;) {
		if (result >= 0) {
			s->_buffer->push(msg->size);

			result = PVDTryPeekFrame(s->_netcmd->_userid, s->_buffer, *msg);
			if (result == 0) {
				ARefsBuf::reserve(s->_buffer, max(512,msg->size), 64*1024);
				result = s->_io->output(msg, s->_buffer);
				if (result == 0) return 0;
				if (result > 0) continue;
			}
			if (result == -EAGAIN) {
				msg->init();
				result = 0;
				continue;
			}
		}
		if (result > 0) {
			TRACE2("recv frame: %x, size = %d.\n", msg->type, msg->size);
			s->_client._main_tick = GetTickCount();

			result = PVDStreamDispatch(s);
		} else {
			result = s->_stream.on_recv(&s->_stream, NULL, result);
		}
		if (result == 0) return 0;
		if ((result < 0) || (result >= AMsgType_Class)) {
			s->_client.use(-1);
			s->release();
			return result;
		}
		msg->init();
	}
}

static int PVDStreamDoRecv(AStreamComponent *sc)
{
	PVDStream *s = container_of(sc, PVDStream, _stream);
	if (s->_status != pvdnet_con_stream)
		return -EACCES;

	s->addref();
	s->_client.use(1);
	s->_heart_msg.init();
	s->_heart_msg.done = PVDStreamOutputDone;
	s->_heart_msg.done2(1);
	return 0;
}

static int PVDStreamOpenDone(AMessage *msg, int result)
{
	PVDStream *s = container_of(msg, PVDStream, _heart_msg);
	result = s->open(result);
	if (result != 0)
		s->_client.exec_done(result);
	return result;
}
static int PVDStreamOpen(AClientComponent *c)
{
	PVDStream *s = container_of(c, PVDStream, _client);
	s->_heart_msg.init();
	s->_heart_msg.done = PVDStreamOpenDone;
	return s->open(1);
}
int PVDStream::open(int result)
{
	while (result > 0) {
	switch (_status)
	{
	case pvdnet_invalid:
		ADeviceComponent *dev;
		if (_netcmd == NULL) {
			if (_stream._dev_id[0] == '\0')
				return -EINVAL;

			ADeviceComponentModule *DCM = ADeviceComponent::get();
			DCM->lock();
			dev = DCM->_find(_stream._dev_id);
			if (dev != NULL) dev->other(&_netcmd);
			if (_netcmd != NULL) _netcmd->_entity->addref();
			DCM->unlock();
			if (_netcmd == NULL)
				return -EINVAL;
		}
		else if (_netcmd->other(&dev) == NULL) {
			return -EINVAL;
		}
		if (_netcmd->_userid == 0) {
			AClientComponent *c;
			if ((_netcmd->other(&c) != NULL) && !c->_auto_reopen)
				_client._auto_reopen = false;
			return -EINVAL;
		}
		if (_io == NULL) {
			if (_io_opt == NULL) {
				char tmp[256];
				snprintf(tmp, sizeof(tmp), "\"async_tcp\": { \"address\":\"%s\", \"port\":%d }",
					dev->_net_addr, dev->_net_port);
				result = AOptionDecode(&_io_opt, tmp, -1);
				if (result < 0)
					return result;
			}
			result = AObject::create(&_io, this, _io_opt, NULL);
			if (result < 0)
				return result;
		}
		_status = pvdnet_connecting;
		_heart_msg.init(_io_opt);
		result = _io->open(&_heart_msg);
		break;
	case pvdnet_connecting:
		result = ARefsBuf::reserve(_buffer, 32*1024, 32*1024);
		if (result < 0)
			return result;
		_buffer->pop(_buffer->len());

		_heart_msg.type = ioMsgType_Block;
		_heart_msg.data = _buffer->next();
		if (_stream._begin_tm.tm_year == 0) {
			_heart_msg.size = PVDCmdEncode(_netcmd->_userid, _heart_msg.data, NET_SDVR_REAL_PLAY, sizeof(STRUCT_SDVR_REALPLAY_EX));

			STRUCT_SDVR_REALPLAY_EX *rt = (STRUCT_SDVR_REALPLAY_EX*)(_heart_msg.data + sizeof(pvdnet_head));
			memzero(*rt);
			rt->byChannel = _stream._chan_id;
			rt->byLinkMode = _stream._stream_id;
		} else {
			_heart_msg.size = PVDCmdEncode(_netcmd->_userid, _heart_msg.data, NETCOM_VOD_RECFILE_REQ_EX, sizeof(STRUCT_SDVR_VOD_EX));

			STRUCT_SDVR_VOD_EX *vod = (STRUCT_SDVR_VOD_EX*)(_heart_msg.data + sizeof(pvdnet_head));
			memzero(*vod);
			vod->byChannel = _stream._chan_id;
			vod->byType = 0xff;
			tm_to_MFS_FIELD_TIME(&_stream._begin_tm, &vod->unBegTime.stFieldTime);
			tm_to_MFS_FIELD_TIME(&_stream._end_tm, &vod->unEndTime.stFieldTime);
			vod->streamtype = _stream._stream_id;
		}
		_status = pvdnet_syn_login;
		result = _io->input(&_heart_msg);
		break;
	case pvdnet_syn_login:
		_status = pvdnet_ack_login;
		result = _io->output(&_heart_msg, _buffer);
		break;
	case pvdnet_ack_login:
		_buffer->push(_heart_msg.size);
		_status = pvdnet_con_stream;

		if (_stream.do_recv == NULL) {
			PVDStreamDoRecv(&_stream);
		}
		return result;
	} }
	return result;
}

static int PVDStreamAbort(AClientComponent *c)
{
	PVDStream *s = container_of(c, PVDStream, _client);
	if (s->_io != NULL)
		s->_io->shutdown();
	return 1;
}

static int PVDStreamClose(AClientComponent *c)
{
	PVDStream *s = container_of(c, PVDStream, _client);
	reset_nif(s->_io, NULL, {
		s->_io->shutdown();
		s->_io->release();
	});
	s->_status = pvdnet_invalid;
	return 1;
}

static int PVDStreamCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDStream *s = (PVDStream*)*object;
	s->init();

	AClientComponent::get()->create((AObject**)&s->_client, s, option);
	s->_client.open = PVDStreamOpen;
	s->_client.abort = PVDStreamAbort;
	s->_client.close = PVDStreamClose;

	AStreamComponent::get()->module.create((AObject**)&s->_stream, s, option);
	s->_stream.do_recv = PVDStreamDoRecv;

	s->_io = NULL;
	s->_io_opt = option->find("io");
	if (s->_io_opt != NULL)
		s->_io_opt = AOptionClone(s->_io_opt, NULL);

	s->_buffer = NULL;
	s->_status = pvdnet_invalid;
	s->_netcmd = NULL;

	if (parent != NULL) {
		((AEntity*)parent)->get(&s->_netcmd);
	}
	if (s->_netcmd != NULL) {
		s->_netcmd->_entity->addref();

		AInOutComponent *iocom = NULL;
		if (s->_netcmd->other(&iocom) != NULL) {
			s->_stream._plugin_mutex = iocom->_mutex;
		}
		ADeviceComponent *dev;
		if (s->_netcmd->other(&dev) != NULL) {
			strcpy_sz(s->_stream._dev_id, dev->_dev_id);
		}
	}
	return 1;
}

static void PVDStreamRelease(AObject *object)
{
	PVDStream *s = (PVDStream*)object;
	s->pop_exit(&s->_client);
	s->pop(&s->_stream);
	AStreamComponent::get()->module.release((AObject*)&s->_stream);
	release_s(s->_io);
	release_s(s->_io_opt);
	release_s(s->_buffer);
	reset_nif(s->_netcmd, NULL, s->_netcmd->_entity->release());
	s->exit();
}

AStreamImplement PVDStreamModule = { {
	"stream",
	"PVDStream",
	sizeof(PVDStream),
	NULL, NULL,
	&PVDStreamCreate,
	&PVDStreamRelease,
},
};
static int reg_s = AModuleRegister(&PVDStreamModule.module);
