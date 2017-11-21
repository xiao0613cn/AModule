// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MQTTCONST_H
#define MQTTCONST_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef __FAILURE__
#include <errno.h>
#define __FAILURE__   (-EFAULT)
#endif

#ifdef __cplusplus
extern "C" {
#else
typedef unsigned char bool;
#define true 1
#define false 0
#endif /* __cplusplus */

//#include "azure_c_shared_utility/crt_abstractions.h"
//#include "umock_c_prod.h"


typedef enum CONTROL_PACKET_TYPE
{
    CONNECT_TYPE = 0x10,
    CONNACK_TYPE = 0x20,
    PUBLISH_TYPE = 0x30,
    PUBACK_TYPE = 0x40,
    PUBREC_TYPE = 0x50,
    PUBREL_TYPE = 0x60,
    PUBCOMP_TYPE = 0x70,
    SUBSCRIBE_TYPE = 0x80,
    SUBACK_TYPE = 0x90,
    UNSUBSCRIBE_TYPE = 0xA0,
    UNSUBACK_TYPE = 0xB0,
    PINGREQ_TYPE = 0xC0,
    PINGRESP_TYPE = 0xD0,
    DISCONNECT_TYPE = 0xE0,
    PACKET_TYPE_ERROR,
    UNKNOWN_TYPE,
} CONTROL_PACKET_TYPE;


typedef enum QOS_VALUE
{
    DELIVER_AT_MOST_ONCE = 0x00,
    DELIVER_AT_LEAST_ONCE = 0x01,
    DELIVER_EXACTLY_ONCE = 0x02,
    DELIVER_FAILURE = 0x80,
} QOS_VALUE;


typedef struct MQTT_CLIENT_OPTIONS
{
    char* clientId;
    char* willTopic;
    char* willMessage;
    char* username;
    char* password;
    uint16_t keepAliveInterval;
    bool messageRetain;
    bool useCleanSession;
    QOS_VALUE qualityOfServiceValue;
    bool log_trace;
} MQTT_CLIENT_OPTIONS;

typedef enum CONNECT_RETURN_CODE
{
    CONNECTION_ACCEPTED = 0x00,
    CONN_REFUSED_UNACCEPTABLE_VERSION = 0x01,
    CONN_REFUSED_ID_REJECTED = 0x02,
    CONN_REFUSED_SERVER_UNAVAIL = 0x03,
    CONN_REFUSED_BAD_USERNAME_PASSWORD = 0x04,
    CONN_REFUSED_NOT_AUTHORIZED = 0x05,
    CONN_REFUSED_UNKNOWN
} CONNECT_RETURN_CODE;

typedef struct CONNECT_ACK
{
    bool isSessionPresent;
    CONNECT_RETURN_CODE returnCode;
} CONNECT_ACK;

typedef struct SUBSCRIBE_PAYLOAD
{
    const char* subscribeTopic;
    QOS_VALUE qosReturn;
} SUBSCRIBE_PAYLOAD;

typedef struct SUBSCRIBE_ACK
{
    uint16_t packetId;
    uint8_t* qosReturn; //QOS_VALUE*
    size_t qosCount;
} SUBSCRIBE_ACK;

typedef struct UNSUBSCRIBE_ACK
{
    uint16_t packetId;
} UNSUBSCRIBE_ACK;

typedef struct PUBLISH_ACK
{
    uint16_t packetId;
} PUBLISH_ACK;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // MQTTCONST_H