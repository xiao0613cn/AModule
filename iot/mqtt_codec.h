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

#ifndef STRING_HANDLE
#define STRING_HANDLE  void*
#define STRING_new(...)      (0)
#define STRING_sprintf(...)  (0)
#define STRING_copy(...)     (0)
#define STRING_concat(...)   (0)
#define STRING_construct_sprintf(...) (0)
#define STRING_concat_with_STRING(...) (0)
#define STRING_delete(...)   (0)
#endif

typedef struct MQTTCODEC_INSTANCE_TAG* MQTTCODEC_HANDLE;

typedef void(*ON_PACKET_COMPLETE_CALLBACK)(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData, void *packetTag);

#ifdef __cplusplus
};

template <typename TObject, void(TObject::*cb)(CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData, void *packetTag)>
void mqtt_packet_callback(void* context, CONTROL_PACKET_TYPE packet, int flags, BUFFER_HANDLE headerData, void *packetTag)
{
	(((TObject*)context)->*cb)(packet, flags, headerData, packetTag);
}

extern "C" {
#endif

MQTTCODEC_HANDLE mqtt_codec_create(ON_PACKET_COMPLETE_CALLBACK packetComplete, void* callbackCtx);
void mqtt_codec_destroy(MQTTCODEC_HANDLE handle);

BUFFER_HANDLE mqtt_codec_connect(BUFFER_HANDLE result/*!=NULL*/, const MQTT_CLIENT_OPTIONS* mqttOptions/*!=NULL*/);
BUFFER_HANDLE mqtt_codec_disconnect(BUFFER_HANDLE result/*!=NULL*/);
BUFFER_HANDLE mqtt_codec_publish(BUFFER_HANDLE result/*!=NULL*/, const struct MQTT_MESSAGE *msg/*!=NULL*/);
BUFFER_HANDLE mqtt_codec_publishAck(uint16_t packetId);
BUFFER_HANDLE mqtt_codec_publishReceived(uint16_t packetId);
BUFFER_HANDLE mqtt_codec_publishRelease(uint16_t packetId);
BUFFER_HANDLE mqtt_codec_publishComplete(uint16_t packetId);
BUFFER_HANDLE mqtt_codec_ping(BUFFER_HANDLE result/*!=NULL*/);
BUFFER_HANDLE mqtt_codec_subscribe(uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count, STRING_HANDLE trace_log);
BUFFER_HANDLE mqtt_codec_unsubscribe(uint16_t packetId, const char** unsubscribeList, size_t count, STRING_HANDLE trace_log);

int mqtt_codec_bytesReceived(MQTTCODEC_HANDLE handle, const unsigned char* buffer, size_t size);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // MQTT_CODEC_H
