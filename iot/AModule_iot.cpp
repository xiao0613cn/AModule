#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
extern "C" {
#include "mqtt_codec.h"
};

void mqtt_packet_done(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData, void *packetTag)
{
	//switch (packet)
	{

	}
	TRACE("recv mqtt packet(%d), flags(%d), buffer size(%d).\n", packet, flags, headerData->size);
}

int test_mqtt()
{
	AObject *tcp = NULL;
	int ret = AObjectCreate(&tcp, NULL, NULL, "tcp");
	if (ret < 0)
		return ret;

	AOption opt;
	AOptionInit(&opt, NULL);

	AOption *addr = AOptionCreate(&opt, "address", "test.mosquitto.org");

	AOption *port = AOptionCreate(&opt, "port", "1883");

	AMessage msg;
	msg.init(&opt);
	ret = tcp->open(tcp, &msg);
	if (ret > 0) {
		MQTT_CLIENT_OPTIONS client = {
			"",
			NULL,
			NULL,
			NULL,
			NULL,
			10,
			false,
			true,
			DELIVER_AT_MOST_ONCE,
			false
		};
		BUFFER_HANDLE buf = mqtt_codec_connect(&client, NULL);

		msg.init(ioMsgType_Block, buf->buffer, buf->size);
		ret = ioInput(tcp, &msg);
		BUFFER_delete(buf);
	}

	ARefsBuf *buf = ARefsBufCreate(64*1024, NULL, NULL);
	MQTTCODEC_HANDLE mqtt = mqtt_codec_create(&mqtt_packet_done, NULL);

	while (ret > 0) {
		msg.init(0, buf->next(), buf->left());
		ret = ioOutput(tcp, &msg);
		if (ret > 0) {
			mqtt_codec_bytesReceived(mqtt, (const uint8_t*)msg.data, msg.size);
		}
	}
	ARefsBufRelease(buf);
	mqtt_codec_destroy(mqtt);
	return ret;
}
