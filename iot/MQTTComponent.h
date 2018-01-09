#ifndef _MQTTCLIENT_COMPONENT_H_
#define _MQTTCLIENT_COMPONENT_H_
#include "mqttconst.h"
#include "mqtt_message.h"
#include "mqtt_codec.h"


struct MqttMsg : public AMessage {
	MQTT_BUFFER buf;
	void  *user;

	static MqttMsg* create() {
		MqttMsg *mm = goarrary(MqttMsg, 1);
		mm->done = &done_free;
		return mm;
	}
	static int done_free(AMessage *msg, int result) {
		MqttMsg *mm = (MqttMsg*)msg;
		BUFFER_unbuild(&mm->buf);
		free(mm);
		return result;
	}
};

struct MqttComponent : public AComponent {
	static const char* name() { return "MqttComponent"; }

	void (*on_msg)(MqttComponent *c, MQTT_MESSAGE *msg);
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
