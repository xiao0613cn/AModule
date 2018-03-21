#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/AClientSystem.h"
#include "../ecs/AInOutComponent.h"
#include "mqtt_codec.h"
#include "mqtt_message.h"
#include "MQTTComponent.h"
#ifdef _WIN32
#pragma comment(lib, "../bin_win32/AModule.lib")
#endif

extern MqttModule MCM;

static int mm_done_free(AMessage *msg, int result)
{
	MqttMsg *mm = (MqttMsg*)msg;
	BUFFER_unbuild(&mm->buf);
	free(mm);
	return result;
}

static MqttMsg* mm_create()
{
	MqttMsg *mm = goarrary(MqttMsg, 1);
	mm->mod = &MCM;
	mm->done = &mm_done_free;
	return mm;
}

struct MQTTClient : public AEntity {
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

	MqttComponent _mqtt;
	AOption   *_options;
	Status     _status;
	AMessage   _heart_msg;

	void init() {
		AEntity::init();
		_init_push(&_client);
		_init_push(&_iocom);
		pthread_mutex_init(&_mutex, NULL);
		_iocom._mutex = &_mutex;

		_init_push(&_mqtt);
		_options = NULL;
		_status = Invalid;
	}
	int open(int result);
	void on_packet(CONTROL_PACKET_TYPE packet, int flags, MQTT_BUFFER* headerData, void *packetTag);
};

static void MQTTInputEnd(AInOutComponent *c, int result)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _iocom);
	mqtt->_client.use(-1);
}

static int MQTTOpenMsgDone(AMessage *msg, int result)
{
	MQTTClient *mqtt = container_of(msg, MQTTClient, _heart_msg);
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
	mqtt->_heart_msg.done = &MQTTOpenMsgDone;
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
		_heart_msg.init(io_opt);
		result = _iocom._io->open(&_heart_msg);
		break;
	}
	case Opening:
	{
		if (result < 0)
			return result;

		if (_mqtt._login.clientId == NULL) {
			AOption *client_opt = _options->find("mqtt_client_options");
			_mqtt._login.clientId = client_opt->getStr("clientId", "");
			_mqtt._login.willTopic = client_opt->getStr("willTopic", NULL);
			_mqtt._login.willMessage = client_opt->getStr("willMessage", NULL);
			_mqtt._login.username = client_opt->getStr("username", NULL);
			_mqtt._login.password = client_opt->getStr("password", NULL);
			_mqtt._login.keepAliveInterval = (uint16_t)client_opt->getInt("keepAliveInterval", 10);
			_mqtt._login.messageRetain = !!client_opt->getInt("messageRetain", false);
			_mqtt._login.useCleanSession = !!client_opt->getInt("useCleanSession", true);
			_mqtt._login.qualityOfServiceValue = (QOS_VALUE)client_opt->getInt("qos", DELIVER_AT_MOST_ONCE);
		}
		MQTT_BUFFER buf = { 0 };
		if (mqtt_codec_connect(&buf, &_mqtt._login) == NULL)
			return -EINVAL;
		_client._tick_heart = _mqtt._login.keepAliveInterval*1000;

		_status = LoginSend;
		_heart_msg.init(ioMsgType_Block, buf.buffer, buf.size);
		result = _iocom._io->input(&_heart_msg);
		break;
	}
	case LoginSend:
	{
		MQTT_BUFFER buf = { (unsigned char*)_heart_msg.data, _heart_msg.size };
		BUFFER_unbuild(&buf);
		if (result >= 0)
			result = ARefsBuf::reserve(_iocom._outbuf, 1024, 0);
		if (result < 0)
			return result;

		mqtt_codec_exit(&_mqtt._codec);
		mqtt_codec_init(&_mqtt._codec, &mqtt_packet_callback<MQTTClient, &MQTTClient::on_packet>, this);
		//_codec = mqtt_codec_create(&mqtt_packet_callback<MQTTClient, &MQTTClient::on_packet>, this);
		//if (_codec == NULL)
		//	return -ENOMEM;

		_status = LoginRecv;
		_iocom._outbuf->reset();
		result = _iocom._io->output(&_heart_msg, _iocom._outbuf);
		break;
	}
	case LoginRecv:
		if (result < 0)
			return result;

		result = mqtt_codec_bytesReceived(&_mqtt._codec, (unsigned char*)_heart_msg.data, _heart_msg.size);
		if ((result != 0) || (_status == LoginFailed))
			return -EFAULT;

		if (_status != Opened) {
			result = _iocom._io->output(&_heart_msg, _iocom._outbuf);
			break;
		}

		_client.use(2);
		_iocom._input_begin(&MQTTInputEnd);
		_iocom._output_cycle(512, 2048);
		return 1;

	default: assert(0); return -EACCES;
	}
	} while (result != 0);
	return 0;
}

void MQTTClient::on_packet(CONTROL_PACKET_TYPE packet, int flags, MQTT_BUFFER* headerData, void *packetTag)
{
	TRACE("packet = %x, flags = %x.\n", packet, flags);

	if (packet == CONNACK_TYPE) {
		CONNECT_ACK *conn_ack = (CONNECT_ACK*)packetTag;
		if (conn_ack->returnCode == CONNECTION_ACCEPTED)
			_status = Opened;
		else
			_status = LoginFailed;
		_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);
		return;
	}
	if (packet == PINGRESP_TYPE) {
		_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);
		return;
	}

	if (packet == PUBLISH_TYPE) {
		MQTT_MESSAGE *msg = (MQTT_MESSAGE*)packetTag;
		// TODO: dispath msg...
		_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);

		if (msg->qosInfo == DELIVER_AT_LEAST_ONCE) {
			MqttMsg *reply = mm_create();
			mqtt_codec_publishAck(&reply->buf, msg->packetId);
			_mqtt.post(reply);
		}
		else if (msg->qosInfo == DELIVER_EXACTLY_ONCE) {
			MqttMsg *reply = mm_create();
			mqtt_codec_publishReceived(&reply->buf, msg->packetId);
			_mqtt.post(reply);
		}
		return;
	}
	if (packet == PUBACK_TYPE) {
		PUBLISH_ACK *publish_ack = (PUBLISH_ACK*)packetTag;
		_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);
		return;
	}
	if (packet == PUBREC_TYPE) {
		PUBLISH_ACK *publish_ack = (PUBLISH_ACK*)packetTag;
		_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);

		MqttMsg *reply = mm_create();
		mqtt_codec_publishRelease(&reply->buf, publish_ack->packetId);
		_mqtt.post(reply);
		return;
	}
	if (packet == PUBREL_TYPE) {
		PUBLISH_ACK *publish_ack = (PUBLISH_ACK*)packetTag;
		_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);

		MqttMsg *reply = mm_create();
		mqtt_codec_publishComplete(&reply->buf, publish_ack->packetId);
		_mqtt.post(reply);
		return;
	}
	_mqtt.on_msg(_mqtt._user, packet, flags, headerData, packetTag);
}

static int MQTTHeartMsgDone(AMessage *msg, int result)
{
#if 0
	MqttMsg *mm = (MqttMsg*)msg;
	MQTTClient *mqtt = (MQTTClient*)mm->user;
	mm->done_free(mm, result);
#else
	MQTTClient *mqtt = container_of(msg, MQTTClient, _heart_msg);

	MQTT_BUFFER buf = { (unsigned char*)msg->data, msg->size };
	BUFFER_unbuild(&buf);
#endif
	mqtt->_client.exec_done(result);
	return result;
}

static int MQTTHeart(AClientComponent *c)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _client);
#if 0
	MqttMsg *mm = mm_create();
	mm->user = mqtt;
	mm->done = &MQTTHeartMsgDone;
	mqtt_codec_ping(&mm->buf);
	mqtt->_mqtt.post(mm);
#else
	MQTT_BUFFER buf = { 0 };
	mqtt_codec_ping(&buf);

	mqtt->_heart_msg.init(ioMsgType_Block, buf.buffer, buf.size);
	mqtt->_heart_msg.done = &MQTTHeartMsgDone;

	mqtt->_iocom.post(&mqtt->_heart_msg);
#endif
	return 0;
}

static int MQTTAbort(AClientComponent *c)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _client);
	mqtt->_iocom._input_end(-EINTR);

	if (mqtt->_iocom._io != NULL)
		mqtt->_iocom._io->shutdown();
	return 1;
}

static int MQTTCloseMsgDone(AMessage *msg, int result)
{
	MQTTClient *mqtt = container_of(msg, MQTTClient, _heart_msg);
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
	mqtt_codec_exit(&mqtt->_mqtt._codec);

	MQTT_BUFFER buf = { 0 };
	mqtt_codec_disconnect(&buf);
	mqtt->_heart_msg.init(ioMsgType_Block, buf.buffer, buf.size);
	mqtt->_heart_msg.done = &MQTTCloseMsgDone;

	int result = mqtt->_iocom._io->input(&mqtt->_heart_msg);
	if (result != 0)
		mqtt->_heart_msg.done2(result);
	return 0;
}

static int MQTTOutput(AInOutComponent *c, int result)
{
	MQTTClient *mqtt = container_of(c, MQTTClient, _iocom);
	if (result >= 0)
		result = mqtt_codec_bytesReceived(&mqtt->_mqtt._codec, (unsigned char*)c->_outbuf->ptr(), c->_outbuf->len());
	if (result != 0) {
		mqtt->_client._main_abort = true;
		mqtt->_client.use(-1);
		mqtt->_iocom._input_end(-EIO);
		return -EIO;
	}

	mqtt->_iocom._outbuf->reset();
	return 1;
}

static void MQTTPost(MqttComponent *c, AMessage *msg)
{
	MQTTClient *p = container_of(c, MQTTClient, _mqtt);
	p->_iocom.post(msg);
}

static void on_msg_null(void* context, CONTROL_PACKET_TYPE packet, int flags, MQTT_BUFFER *headerData, void *packetTag)
{
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
	mqtt->_iocom.on_output = &MQTTOutput;
	mqtt->_mqtt.on_msg = &on_msg_null;
	mqtt->_mqtt.do_post = &MQTTPost;
	return 1;
}

static void MQTTRelease(AObject *object)
{
	MQTTClient *mqtt = (MQTTClient*)object;
	mqtt->_pop_exit(&mqtt->_client);
	mqtt->_pop_exit(&mqtt->_iocom);
	pthread_mutex_destroy(&mqtt->_mutex);

	mqtt->_pop_exit(&mqtt->_mqtt);
	release_s(mqtt->_options);
	mqtt->exit();
}

MqttModule MCM = { {
	"AEntity",
	"MQTTClient",
	sizeof(MQTTClient),
	NULL, NULL,
	&MQTTCreate,
	&MQTTRelease,
},
	&mm_create, (void(*)(MqttMsg*))&free,

	&BUFFER_pre_build, &BUFFER_build,
	&BUFFER_unbuild,
	&BUFFER_enlarge, &BUFFER_append, &BUFFER_prepend,

	&mqtt_codec_init, &mqtt_codec_exit, &mqtt_codec_bytesReceived,
	&mqtt_codec_connect, &mqtt_codec_disconnect,
	&mqtt_codec_publish, &mqtt_codec_publishAck,
	&mqtt_codec_publishReceived, &mqtt_codec_publishComplete, &mqtt_codec_publishRelease,
	&mqtt_codec_ping, &mqtt_codec_subscribe, &mqtt_codec_unsubscribe,
};

static int reg_mqtt = AModuleRegister(&MCM.module);
