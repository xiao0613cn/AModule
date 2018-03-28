#ifndef _MQTTCLIENT_COMPONENT_H_
#define _MQTTCLIENT_COMPONENT_H_
#include "mqttconst.h"
#include "mqtt_message.h"
#include "mqtt_codec.h"
#include "../ecs/AEntity.h"


typedef struct MqttMsg MqttMsg;

struct MqttModule {
	AModule module;
	AMODULE_GET(MqttModule, "AEntity", "MQTTClient")

	MqttMsg*  (*msg_create)();
	void      (*msg_release)(MqttMsg *msg);

	int  (*buf_pre_build)(MQTT_BUFFER *handle, size_t size);
	int  (*buf_build)(MQTT_BUFFER *handle, const unsigned char* source, size_t size);
	int  (*buf_unbuild)(MQTT_BUFFER *handle);
	int  (*buf_enlarge)(MQTT_BUFFER *handle, size_t enlargeSize);
	int  (*buf_append)(MQTT_BUFFER *handle, const void *buffer, size_t size);
	int  (*buf_prepend)(MQTT_BUFFER *handle, const void *buffer, size_t size);

	void (*codec_init)(MQTTCODEC_INSTANCE *handle, ON_PACKET_COMPLETE_CALLBACK packetComplete, void* callbackCtx);
	void (*codec_exit)(MQTTCODEC_INSTANCE *handle);
	int  (*codec_bytesReceived)(MQTTCODEC_INSTANCE *handle/*!=NULL*/, const unsigned char* buffer/*!=NULL*/, size_t size/*!=0*/);

	MQTT_BUFFER* (*codec_connect)(MQTT_BUFFER *result/*!=NULL*/, const MQTT_CLIENT_OPTIONS* mqttOptions/*!=NULL*/);
	MQTT_BUFFER* (*codec_disconnect)(MQTT_BUFFER *result/*!=NULL*/);
	MQTT_BUFFER* (*codec_publish)(MQTT_BUFFER *result/*!=NULL*/, const MQTT_MESSAGE *msg/*!=NULL*/);
	MQTT_BUFFER* (*codec_publishAck)(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
	MQTT_BUFFER* (*codec_publishReceived)(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
	MQTT_BUFFER* (*codec_publishRelease)(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
	MQTT_BUFFER* (*codec_publishComplete)(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
	MQTT_BUFFER* (*codec_ping)(MQTT_BUFFER *result/*!=NULL*/);
	MQTT_BUFFER* (*codec_subscribe)(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count);
	MQTT_BUFFER* (*codec_unsubscribe)(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId, const char** unsubscribeList, size_t count);
};

struct MqttMsg : public AMessage {
	MQTT_BUFFER buf;
	void       *user;
};

struct MqttComponent : public AComponent {
	static const char* name() { return "MqttComponent"; }

	//void (*on_msg)(MqttComponent *c, MQTT_MESSAGE *msg);
	ON_PACKET_COMPLETE_CALLBACK on_msg;
	void  *_user;

	void post(MqttMsg *msg) {
		if (msg->data == NULL)
			msg->init(ioMsgType_Block, msg->buf.buffer, msg->buf.size);
		do_post(this, msg);
	}
	void  (*do_post)(MqttComponent *c, AMessage *msg); // => AInOutComponent.do_post()

	MQTTCODEC_INSTANCE _codec;
	MQTT_CLIENT_OPTIONS _login;

	void init2() {
		on_msg = NULL; _user = NULL; do_post = NULL;
		mqtt_codec_init(&_codec, NULL, NULL);
		memzero(_login);
	}
	void exit2() {
		mqtt_codec_exit(&_codec);
	}
};

#endif
