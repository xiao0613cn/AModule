// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTT_MESSAGE_H
#define MQTT_MESSAGE_H

#ifdef __cplusplus
#include <cstdint>
#include <cstdbool>
#include <cstddef>
extern "C" {
#else
//#include <stdbool.h>
#include <stddef.h>
#endif // __cplusplus

#include "mqttconst.h"
//#include "azure_c_shared_utility/umock_c_prod.h"

typedef struct APP_PAYLOAD_TAG
{
	uint8_t* message;
	size_t length;
} APP_PAYLOAD;

typedef struct MQTT_MESSAGE_TAG
{
	uint16_t packetId;
	char* topicName;
	QOS_VALUE qosInfo;
	APP_PAYLOAD appPayload;
	bool isDuplicateMsg;
	bool isMessageRetained;
} MQTT_MESSAGE, * MQTT_MESSAGE_HANDLE;


MOCKABLE_FUNCTION(, MQTT_MESSAGE_HANDLE, mqttmessage_create, uint16_t packetId, const char* topicName, QOS_VALUE qosValue, const uint8_t* appMsg, size_t appMsgLength);
MOCKABLE_FUNCTION(,void, mqttmessage_destroy, MQTT_MESSAGE_HANDLE handle);
MOCKABLE_FUNCTION(,MQTT_MESSAGE_HANDLE, mqttmessage_clone, MQTT_MESSAGE_HANDLE handle);

MOCKABLE_FUNCTION(, uint16_t, mqttmessage_getPacketId, MQTT_MESSAGE_HANDLE handle);
MOCKABLE_FUNCTION(, const char*, mqttmessage_getTopicName, MQTT_MESSAGE_HANDLE handle);
MOCKABLE_FUNCTION(, QOS_VALUE, mqttmessage_getQosType, MQTT_MESSAGE_HANDLE handle);
MOCKABLE_FUNCTION(, bool, mqttmessage_getIsDuplicateMsg, MQTT_MESSAGE_HANDLE handle);
MOCKABLE_FUNCTION(, bool, mqttmessage_getIsRetained, MQTT_MESSAGE_HANDLE handle);
MOCKABLE_FUNCTION(, int, mqttmessage_setIsDuplicateMsg, MQTT_MESSAGE_HANDLE handle, bool duplicateMsg);
MOCKABLE_FUNCTION(, int, mqttmessage_setIsRetained, MQTT_MESSAGE_HANDLE handle, bool retainMsg);
MOCKABLE_FUNCTION(, const APP_PAYLOAD*, mqttmessage_getApplicationMsg, MQTT_MESSAGE_HANDLE handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MQTT_MESSAGE_H
