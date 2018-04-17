// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
//#include <cstdint>
//#include <cstdbool>
//#include <cstddef>
extern "C" {
#else
//#include <stdbool.h>
#endif // __cplusplus

#include "mqttconst.h"
//#include "azure_c_shared_utility/umock_c_prod.h"

typedef struct APP_PAYLOAD_TAG
{
	uint8_t* message;
	size_t length;
} APP_PAYLOAD;

typedef struct PUBLISH_MSG
{
	uint16_t    packetId;
	uint16_t    topicLen;
	char*       topicName;
	QOS_VALUE   qosInfo;
	APP_PAYLOAD appPayload;
	bool        isDuplicate;
	bool        isRetained;
} PUBLISH_MSG, * MQTT_MESSAGE_HANDLE;


MQTT_MESSAGE_HANDLE mqttmessage_create(uint16_t packetId, const char* topicName, QOS_VALUE qosValue, const uint8_t* appMsg, size_t appMsgLength);
void mqttmessage_destroy(MQTT_MESSAGE_HANDLE handle);
MQTT_MESSAGE_HANDLE mqttmessage_clone(MQTT_MESSAGE_HANDLE handle);

uint16_t mqttmessage_getPacketId(MQTT_MESSAGE_HANDLE handle);
const char* mqttmessage_getTopicName(MQTT_MESSAGE_HANDLE handle);
QOS_VALUE mqttmessage_getQosType(MQTT_MESSAGE_HANDLE handle);
bool mqttmessage_getIsDuplicateMsg(MQTT_MESSAGE_HANDLE handle);
bool mqttmessage_getIsRetained(MQTT_MESSAGE_HANDLE handle);
int mqttmessage_setIsDuplicateMsg(MQTT_MESSAGE_HANDLE handle, bool duplicateMsg);
int mqttmessage_setIsRetained(MQTT_MESSAGE_HANDLE handle, bool retainMsg);
const APP_PAYLOAD* mqttmessage_getApplicationMsg(MQTT_MESSAGE_HANDLE handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTT_MESSAGE_H
