#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "mqtt_codec.h"
#include "../ecs/AClientSystem.h"
#include "../ecs/AInOutComponent.h"

struct MQTTClient : public AEntity2 {
	enum Status {
		Invalid = 0,
		Opening,
		LoginSend,
		LoginRecv,
		LoginFailed,
		Opened,
	};

	AClientComponent _client;
	AInOutComponent  _iocom;
	pthread_mutex_t  _mutex;

	MQTTCODEC_HANDLE _codec;
	AOption   *_options;
	Status     _status;
	AMessage   _heart_msg;

	void init() {
		AEntity2::init();
		_client.init(this); _push(&_client);
		_iocom.init(this); _push(&_iocom);
		pthread_mutex_init(&_mutex, NULL);
		_iocom._mutex = &_mutex;

		_codec = NULL;
		_options = NULL;
		_status = Invalid;
	}
	int open(int result);
	void on_packet(CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData, void *packetTag);
};

static int MQTTOpenMsgDone(AMessage *msg, int result)
{
	MQTTClient *mqtt = container_of(msg, MQTTClient, _iocom._outmsg);
	result = mqtt->open(result);
	if (result != 0)
		mqtt->_client.exec_done(result);
	return result;
}

static int MQTTOpen(AClientComponent *c)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _client);
	if (!mqtt->_iocom._queue.empty()) {
		assert(0);
		return -EBUSY;
	}
	mqtt->_iocom._outmsg.done = &MQTTOpenMsgDone;
	return mqtt->open(0);
}

int MQTTClient::open(int result)
{
	do {
	switch (_status)
	{
	case Invalid:
	{
		AOption *io_opt = _options->find("io");
		if (_iocom._io == NULL) {
			result = AObject::create(&_iocom._io, this, io_opt, "async_tcp");
			if (result < 0)
				return result;
		}
		_status = Opening;
		_iocom._outmsg.init(io_opt);
		result = _iocom._io->open(&_iocom._outmsg);
		break;
	}
	case Opening:
	{
		if (result < 0)
			return result;

		AOption *client_opt = _options->find("mqtt_client_options");
		MQTT_CLIENT_OPTIONS login_opt = { 0 };
		login_opt.clientId = client_opt->getStr("clientId", NULL);
		login_opt.willTopic = client_opt->getStr("willTopic", NULL);
		login_opt.willMessage = client_opt->getStr("willMessage", NULL);
		login_opt.username = client_opt->getStr("username", NULL);
		login_opt.password = client_opt->getStr("password", NULL);
		login_opt.keepAliveInterval = (uint16_t)client_opt->getInt("keepAliveInterval", 10);
		login_opt.messageRetain = (bool)client_opt->getInt("messageRetain", false);
		login_opt.useCleanSession = (bool)client_opt->getInt("useCleanSession", true);
		login_opt.qualityOfServiceValue = (QOS_VALUE)client_opt->getInt("qos", DELIVER_AT_MOST_ONCE);

		MQTT_BUFFER buf = { 0 };
		if (mqtt_codec_connect(&buf, &login_opt) == NULL)
			return -EINVAL;

		_client._tick_heart = login_opt.keepAliveInterval*1000;
		_status = LoginSend;
		_iocom._outmsg.init(ioMsgType_Block, buf.buffer, buf.size);
		result = _iocom._io->input(&_iocom._outmsg);
		break;
	}
	case LoginSend:
	{
		MQTT_BUFFER buf = { (unsigned char*)_iocom._outmsg.data, _iocom._outmsg.size };
		BUFFER_unbuild(&buf);
		if (result >= 0)
			result = ARefsBuf::reserve(_iocom._outbuf, 1024, 0);
		if (result < 0)
			return result;

		_codec = mqtt_codec_create(&mqtt_packet_callback<MQTTClient, &MQTTClient::on_packet>, this);
		if (_codec == NULL)
			return -ENOMEM;

		_status = LoginRecv;
		_iocom._outbuf->reset();
		result = _iocom._io->output(&_iocom._outmsg, _iocom._outbuf);
		break;
	}
	case LoginRecv:
		if (result < 0)
			return result;

		_iocom._outbuf->push(_iocom._outmsg.size);
		result = mqtt_codec_bytesReceived(_codec, (unsigned char*)_iocom._outbuf->ptr(), _iocom._outbuf->len());
		if (result != 0)
			return -EFAULT;
		result = 1;
		break;
	case LoginFailed:
		return -EFAULT;
	case Opened:
		return 1;
	default: assert(0); return -EACCES;
	}
	} while (result != 0);
	return 0;
}

static int MQTTHeartMsgDone(AMessage *msg, int result)
{
	MQTTClient *mqtt = container_of(msg, MQTTClient, _heart_msg);

	MQTT_BUFFER buf = { (unsigned char*)msg->data, msg->size };
	BUFFER_unbuild(&buf);

	mqtt->_client.exec_done(result);
	return result;
}

static int MQTTHeart(AClientComponent *c)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _client);

	MQTT_BUFFER buf = { 0 };
	mqtt_codec_ping(&buf);

	mqtt->_heart_msg.init(ioMsgType_Block, buf.buffer, buf.size);
	mqtt->_heart_msg.done = &MQTTHeartMsgDone;

	mqtt->_iocom.post(&mqtt->_heart_msg);
	return 0;
}

static int MQTTAbort(AClientComponent *c)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _client);
	mqtt->_iocom._abort = true;
	if (mqtt->_iocom._io != NULL)
		mqtt->_iocom._io->shutdown();
	return 1;
}

static int MQTTCloseMsgDone(AMessage *msg, int result)
{
	MQTTClient *mqtt = container_of(msg, MQTTClient, _iocom._outmsg);
	if (msg->data != NULL) {
		MQTT_BUFFER buf = { (unsigned char*)msg->data, msg->size };
		BUFFER_unbuild(&buf);

		msg->init();
		result = mqtt->_iocom._io->close(msg);
		if (result == 0)
			return 0;
	}
	mqtt->_client.exec_done(result);
	return result;
}

static int MQTTClose(AClientComponent *c)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _client);
	if (mqtt->_iocom._io == NULL)
		return 1;

	mqtt->_status = MQTTClient::Invalid;
	if_not(mqtt->_codec, NULL, mqtt_codec_destroy);

	MQTT_BUFFER buf = { 0 };
	mqtt_codec_disconnect(&buf);
	mqtt->_iocom._outmsg.init(ioMsgType_Block, buf.buffer, buf.size);
	mqtt->_iocom._outmsg.done = &MQTTCloseMsgDone;

	int result = mqtt->_iocom._io->input(&mqtt->_iocom._outmsg);
	if (result != 0)
		mqtt->_iocom._outmsg.done2(result);
	return 0;
}

static void MQTTInputEnd(AInOutComponent *c, int result)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _iocom);
	mqtt->_client.use(-1);
}

static int MQTTOutput(AInOutComponent *c, int result)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _iocom);

}

static int MQTTCreate(AObject **object, AObject *parent, AOption *option)
{
	MQTTClient *mqtt = (MQTTClient*)*object;
	mqtt->init();
	mqtt->_options = AOptionClone(option, NULL);
	//AOption *conn_opt = option->find("mqtt_client_options");
	//mqtt->_client._tick_heart = conn_opt->getInt("keepAliveInterval", 10)*1000;

	mqtt->_client.open = &MQTTOpen;
	mqtt->_client.heart = &MQTTHeart;
	mqtt->_client.abort = &MQTTAbort;
	mqtt->_client.close = &MQTTClose;
	mqtt->_iocom.on_end = &MQTTInputEnd;
	mqtt->_iocom.on_output = &MQTTOutput;
	return 1;
}

static void MQTTRelease(AObject *object)
{
	MQTTClient *mqtt = (MQTTClient*)object;
	mqtt->_pop(&mqtt->_client);
	mqtt->_pop(&mqtt->_iocom);
	release_s(mqtt->_iocom._io);
	release_s(mqtt->_iocom._outbuf);

	pthread_mutex_destroy(&mqtt->_mutex);
	release_s(mqtt->_options);
	mqtt->exit();
}

AModule MQTTClientModule = {
	"AEntity2",
	"MQTTClient",
	sizeof(MQTTClient),
	NULL, NULL,
	&MQTTCreate,
	&MQTTRelease,
};

static auto_reg_t reg(MQTTClientModule);
