// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_CODEC_H
#define MQTT_CODEC_H

#include <stdint.h>
//#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#else
#endif // __cplusplus

//#include "azure_c_shared_utility/xio.h"
#include "buffer_.h"
//#include "azure_c_shared_utility/umock_c_prod.h"
#include "mqttconst.h"
//#include "azure_c_shared_utility/strings.h"


typedef enum CODEC_STATE_RESULT {
	CODEC_STATE_FIXED_HEADER,
	CODEC_STATE_VAR_HEADER,
	CODEC_STATE_PAYLOAD,
} CODEC_STATE_RESULT;

typedef void(*ON_PACKET_COMPLETE_CALLBACK)(void* context, CONTROL_PACKET_TYPE packet, int flags, MQTT_BUFFER *headerData, void *packetTag);
typedef struct MQTT_MESSAGE MQTT_MESSAGE;

typedef struct MQTTCODEC_INSTANCE {
	CONTROL_PACKET_TYPE currPacket;
	CODEC_STATE_RESULT codecState;
	size_t bufferOffset;
	int headerFlags;
	MQTT_BUFFER headerData;
	ON_PACKET_COMPLETE_CALLBACK packetComplete;
	void* callContext;
	uint8_t storeRemainLen[4];
	size_t remainLenIndex;
} MQTTCODEC_INSTANCE;

#ifdef __cplusplus
};

template <typename AType, void(AType::*cb)(CONTROL_PACKET_TYPE packet, int flags, MQTT_BUFFER *headerData, void *packetTag)>
void mqtt_packet_callback(void* context, CONTROL_PACKET_TYPE packet, int flags, MQTT_BUFFER *headerData, void *packetTag) {
	(((AType*)context)->*cb)(packet, flags, headerData, packetTag);
}

extern "C" {
#endif

void mqtt_codec_init(MQTTCODEC_INSTANCE *handle, ON_PACKET_COMPLETE_CALLBACK packetComplete, void* callbackCtx);
void mqtt_codec_exit(MQTTCODEC_INSTANCE *handle);

MQTT_BUFFER* mqtt_codec_connect(MQTT_BUFFER *result/*!=NULL*/, const MQTT_CLIENT_OPTIONS* mqttOptions/*!=NULL*/);
MQTT_BUFFER* mqtt_codec_disconnect(MQTT_BUFFER *result/*!=NULL*/);
MQTT_BUFFER* mqtt_codec_publish(MQTT_BUFFER *result/*!=NULL*/, const MQTT_MESSAGE *msg/*!=NULL*/);
MQTT_BUFFER* mqtt_codec_publishAck(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
MQTT_BUFFER* mqtt_codec_publishReceived(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
MQTT_BUFFER* mqtt_codec_publishRelease(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
MQTT_BUFFER* mqtt_codec_publishComplete(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId);
MQTT_BUFFER* mqtt_codec_ping(MQTT_BUFFER *result/*!=NULL*/);
MQTT_BUFFER* mqtt_codec_subscribe(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count);
MQTT_BUFFER* mqtt_codec_unsubscribe(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId, const char** unsubscribeList, size_t count);

int mqtt_codec_bytesReceived(MQTTCODEC_INSTANCE *handle/*!=NULL*/, const unsigned char* buffer/*!=NULL*/, size_t size/*!=0*/);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // MQTT_CODEC_H
