#include "../stdafx.h"
#include "../ecs/AClientSystem.h"
#include "../device_agent/stream.h"
#include "../device_agent/device.h"
#include "PvdNetCmd.h"


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
static inline void tm_to_MFS_FIELD_TIME(struct tm *tm, MFS_FIELD_TIME *ut)
{
	ut->nYear   = tm->tm_year + 1900 - 2000;
	ut->uMonth  = tm->tm_mon + 1;
	ut->uDay    = tm->tm_mday;
	ut->uHour   = tm->tm_hour;
	ut->uMinute = tm->tm_min;
	ut->uSecond = tm->tm_sec;
}
int PVDStream::open(int result)
{
	do {
	if (_heart_msg.size == 0) {
		ADeviceComponent *dev = NULL;
		if (_netcmd == NULL) {
			if (_stream._dev_id[0] == '\0')
				return -EINVAL;

			ADeviceComponentModule *dcm = ADeviceComponent::get();
			dcm->lock();
			dev = dcm->_find(_stream._dev_id);
			if (dev != NULL) dev->other(&_netcmd);
			if (_netcmd != NULL) _netcmd->_entity->addref();
			dcm->unlock();

			if (_netcmd == NULL)
				return -EINVAL;
		}
		else if (_netcmd->other(&dev) == NULL) {
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
		_heart_msg.init(_io_opt);
		result = _io->open(&_heart_msg);
		continue;
	}
	if (_heart_msg.type == AMsgType_AOption) {
		result = ARefsBuf::reserve(_buffer, 32*1024, 32*1024);
		if (result < 0)
			return result;

		_heart_msg.type = ioMsgType_Block;
		_heart_msg.data = _buffer.ptr();
		if (_stream._begin_tm.tm_year == 0) {
			_heart_msg.size = PVDCmdEncode(_netcmd->_userid, _heart_msg.data, NET_SDVR_REAL_PLAY, sizeof(STRUCT_SDVR_REALPLAY_EX));

			STRUCT_SDVR_REALPLAY_EX *rt = (STRUCT_SDVR_REALPLAY_EX*)_heart_msg.data;
			memzero(*rt);
			rt->byChannel = _stream._chan_id;
			rt->byLinkMode = _stream._stream_id;
		} else {
			_heart_msg.size = PVDCmdEncode(_netcmd->_userid, _heart_msg.data, NETCOM_VOD_RECFILE_REQ_EX, sizeof(STRUCT_SDVR_VOD_EX));

			STRUCT_SDVR_VOD_EX *vod = (STRUCT_SDVR_VOD_EX*)_heart_msg.data;
			memzero(*vod);
			vod->byChannel = _stream._chan_id;
			vod->byType = 0xff;
			tm_to_MFS_FIELD_TIME(&_stream._begin_tm, &vod->unBegTime.stFieldTime);
			tm_to_MFS_FIELD_TIME(&_stream._end_tm, &vod->unEndTime.stFieldTime);
			vod->streamtype = _stream._stream_id;
		}
		result = _io->input(&_heart_msg);
		continue;
	}
	
	} while (result > 0);
	return result;
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
	s->_io = NULL;
	s->_io_opt = option->find("io");
	if (s->_io_opt != NULL)
		s->_io_opt = AOptionClone(s->_io_opt, NULL);

	s->_buffer = NULL;
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
	}
	return 1;
}
