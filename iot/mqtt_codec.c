// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <limits.h>
#include <string.h>
//#include "azure_c_shared_utility/optimize_size.h"
//#include "azure_c_shared_utility/gballoc.h"
#include "buffer_.h"
//#include "azure_c_shared_utility/strings.h"
//#include "azure_c_shared_utility/macro_utils.h"
//#include "azure_c_shared_utility/xlogging.h"
#include "mqtt_codec.h"
#include "mqtt_message.h"
//#include <inttypes.h>

#define VARIABLE_HEADER_OFFSET          2
#define PAYLOAD_OFFSET                      5
#define PACKET_TYPE_BYTE(p)                 ((uint8_t)(((uint8_t)(p)) & 0xf0))
#define FLAG_VALUE_BYTE(p)                  ((uint8_t)(((uint8_t)(p)) & 0xf))

#define USERNAME_FLAG                       0x80
#define PASSWORD_FLAG                       0x40
#define WILL_RETAIN_FLAG                    0x20
#define WILL_QOS_FLAG_                      0x18
#define WILL_FLAG_FLAG                      0x04
#define CLEAN_SESSION_FLAG                  0x02

#define NEXT_128_CHUNK                      0x80
#define PROTOCOL_NUMBER                     4
#define CONN_FLAG_BYTE_OFFSET               7

#define CONNECT_FIXED_HEADER_SIZE           2
#define CONNECT_VARIABLE_HEADER_SIZE        10

enum PUBLISH_MSG_FLAGS {
	PUBLISH_QOS_RETAIN =         0x1,
	PUBLISH_QOS_AT_LEAST_ONCE =  0x2,
	PUBLISH_QOS_EXACTLY_ONCE =   0x4,
	PUBLISH_DUP_FLAG =           0x8,
};

#define SUBSCRIBE_FIXED_HEADER_FLAG         0x2
#define UNSUBSCRIBE_FIXED_HEADER_FLAG       0x2

#define MAX_SEND_SIZE                       0xFFFFFF7F


static const char* TRUE_CONST = "true";
static const char* FALSE_CONST = "false";


static const char* retrieve_qos_value(QOS_VALUE value)
{
    switch (value)
    {
        case DELIVER_AT_MOST_ONCE:
            return "DELIVER_AT_MOST_ONCE";
        case DELIVER_AT_LEAST_ONCE:
            return "DELIVER_AT_LEAST_ONCE";
        case DELIVER_EXACTLY_ONCE:
        default:
            return "DELIVER_EXACTLY_ONCE";
    }
}

static void byteutil_writeByte(uint8_t** buffer, uint8_t value)
{
        **buffer = value;
        (*buffer)++;
}

static void byteutil_write_uint16(uint8_t** buffer, uint16_t value)
{
        **buffer = (char)(value / 256);
        (*buffer)++;
        **buffer = (char)(value % 256);
        (*buffer)++;
}

static void byteutil_writeUTF(uint8_t** buffer, const char* stringData, uint16_t len)
{
        byteutil_write_uint16(buffer, len);
        memcpy(*buffer, stringData, len);
        *buffer += len;
}

static uint8_t byteutil_readByte(uint8_t** buffer)
{
	uint8_t result = **buffer;
	(*buffer)++;
	return result;
}

static uint16_t byteutil_read_uint16(uint8_t** buffer, size_t len)
{
	uint16_t result = 0;
	if (len >= 2)
	{
		result = 256 * (**buffer) + (*(*buffer + 1));
		*buffer += 2; // Move the ptr
	}
	else
	{
		//LOG(AZ_LOG_ERROR, LOG_LINE, "byteutil_read_uint16 == NULL or less than 2");
	}
	return result;
}

static char* byteutil_readUTF(uint8_t** buffer, uint16_t* utfLen)
{
	char* result = NULL;
	// Get the length of the string
	uint16_t len = byteutil_read_uint16(buffer, 2);
	if (utfLen != NULL)
		*utfLen = len;
	if (len > 0)
	{
		result = (char*)malloc(len + 1);
		if (result != NULL)
		{
			(void)memcpy(result, *buffer, len);
			result[len] = '\0';
			*buffer += len;
		}
	}
	return result;
}

CONTROL_PACKET_TYPE processControlPacketType(uint8_t pktByte, int* flags)
{
    CONTROL_PACKET_TYPE result;
    result = PACKET_TYPE_BYTE(pktByte);
    if (flags != NULL)
    {
        *flags = FLAG_VALUE_BYTE(pktByte);
    }
    return result;
}

static int addListItemsToUnsubscribePacket(MQTT_BUFFER* ctrlPacket, const char** payloadList, size_t payloadCount)
{
    int result = 0;
        size_t index = 0;
        for (index = 0; index < payloadCount && result == 0; index++)
        {
            // Add the Payload
            size_t offsetLen = BUFFER_length(ctrlPacket);
            size_t topicLen = strlen(payloadList[index]);
            if (topicLen > USHRT_MAX)
            {
                result = __FAILURE__;
            }
            else if (BUFFER_enlarge(ctrlPacket, topicLen + 2) != 0)
            {
                result = __FAILURE__;
            }
            else
            {
                uint8_t* iterator = BUFFER_u_char(ctrlPacket);
                iterator += offsetLen;
                byteutil_writeUTF(&iterator, payloadList[index], (uint16_t)topicLen);
            }
        }
    return result;
}

static int addListItemsToSubscribePacket(MQTT_BUFFER* ctrlPacket, SUBSCRIBE_PAYLOAD* payloadList, size_t payloadCount)
{
    int result = 0;
        size_t index = 0;
        for (index = 0; index < payloadCount && result == 0; index++)
        {
            // Add the Payload
            size_t offsetLen = BUFFER_length(ctrlPacket);
            size_t topicLen = strlen(payloadList[index].subscribeTopic);
            if (topicLen > USHRT_MAX)
            {
                result = __FAILURE__;
            }
            else if (BUFFER_enlarge(ctrlPacket, topicLen + 2 + 1) != 0)
            {
                result = __FAILURE__;
            }
            else
            {
                uint8_t* iterator = BUFFER_u_char(ctrlPacket);
                iterator += offsetLen;
                byteutil_writeUTF(&iterator, payloadList[index].subscribeTopic, (uint16_t)topicLen);
                *iterator = payloadList[index].qosReturn;
            }
        }
    return result;
}

static int constructConnectVariableHeader(MQTT_BUFFER* ctrlPacket, const MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result = 0;
    if (BUFFER_enlarge(ctrlPacket, CONNECT_VARIABLE_HEADER_SIZE) != 0)
    {
        result = __FAILURE__;
    }
    else
    {
        uint8_t* iterator = BUFFER_u_char(ctrlPacket);
        if (iterator == NULL)
        {
            result = __FAILURE__;
        }
        else
        {
            byteutil_writeUTF(&iterator, "MQTT", 4);
            byteutil_writeByte(&iterator, PROTOCOL_NUMBER);
            byteutil_writeByte(&iterator, 0); // Flags will be entered later
            byteutil_write_uint16(&iterator, mqttOptions->keepAliveInterval);
            result = 0;
        }
    }
    return result;
}

static int constructPublishVariableHeader(MQTT_BUFFER* ctrlPacket, const PUBLISH_MSG *msg/*!=NULL*/)
{
    int result = 0;
    size_t topicLen = strlen(msg->topicName);
    size_t spaceLen = 2;
    size_t idLen = 0;

    size_t currLen = BUFFER_length(ctrlPacket);

	// Packet Id is only set if the QOS is not 0
    if (msg->qosInfo != DELIVER_AT_MOST_ONCE)
        idLen = 2;

    if (topicLen > USHRT_MAX)
    {
        result = __FAILURE__;
    }
    else if (BUFFER_enlarge(ctrlPacket, topicLen + idLen + spaceLen) != 0)
    {
        result = __FAILURE__;
    }
    else
    {
        uint8_t* iterator = BUFFER_u_char(ctrlPacket);
        iterator += currLen;
        /* The Topic Name MUST be present as the first field in the PUBLISH Packet Variable header.
		   It MUST be 792 a UTF-8 encoded string [MQTT-3.3.2-1] as defined in section 1.5.3.*/
        byteutil_writeUTF(&iterator, msg->topicName, (uint16_t)topicLen);

        if (idLen > 0)
            byteutil_write_uint16(&iterator, msg->packetId);
        result = 0;
    }
    return result;
}

static int constructSubscibeTypeVariableHeader(MQTT_BUFFER* ctrlPacket, uint16_t packetId)
{
    int result = 0;
    if (BUFFER_enlarge(ctrlPacket, 2) != 0)
    {
        result = __FAILURE__;
    }
    else
    {
        uint8_t* iterator = BUFFER_u_char(ctrlPacket);
        if (iterator == NULL)
        {
            result = __FAILURE__;
        }
        else
        {
            byteutil_write_uint16(&iterator, packetId);
            result = 0;
        }
    }
    return result;
}

static MQTT_BUFFER* constructPublishReply(MQTT_BUFFER *result/*!=NULL*/, CONTROL_PACKET_TYPE type, uint8_t flags, uint16_t packetId)
{
        if (BUFFER_pre_build(result, 4) != 0)
        {
            BUFFER_unbuild(result);
            result = NULL;
        }
        else
        {
            uint8_t* iterator = BUFFER_u_char(result);
            if (iterator == NULL)
            {
                BUFFER_unbuild(result);
                result = NULL;
            }
            else
            {
                *iterator = (uint8_t)type | flags;
                iterator++;
                *iterator = 0x2;
                iterator++;
                byteutil_write_uint16(&iterator, packetId);
            }
        }
    return result;
}

static int constructFixedHeader(MQTT_BUFFER* ctrlPacket, CONTROL_PACKET_TYPE packetType, uint8_t flags)
{
    size_t packetLen = BUFFER_length(ctrlPacket);
	size_t index = 0;
	uint8_t fixedHeader[6];

	fixedHeader[index++] = (uint8_t)packetType | flags;

    // Calculate the length of packet
    do {
        uint8_t encode = packetLen % 128;
        packetLen /= 128;
        // if there are more data to encode, set the top bit of this byte
        if (packetLen > 0) {
            encode |= NEXT_128_CHUNK;
        }
        fixedHeader[index++] = encode;
    } while (packetLen > 0);

    return BUFFER_prepend(ctrlPacket, fixedHeader, index);
}

static int constructConnPayload(MQTT_BUFFER* ctrlPacket, const MQTT_CLIENT_OPTIONS* mqttOptions)
{
    int result = 0;

    size_t clientLen = 0;
    size_t usernameLen = 0;
    size_t passwordLen = 0;
    size_t willMessageLen = 0;
    size_t willTopicLen = 0;
    size_t spaceLen = 0;
	size_t currLen;
	size_t totalLen;

    if (mqttOptions->clientId != NULL)
    {
        spaceLen += 2;
        clientLen = strlen(mqttOptions->clientId);
    }
    if (mqttOptions->username != NULL)
    {
        spaceLen += 2;
        usernameLen = strlen(mqttOptions->username);
    }
    if (mqttOptions->password != NULL)
    {
        spaceLen += 2;
        passwordLen = strlen(mqttOptions->password);
    }
    if (mqttOptions->willMessage != NULL)
    {
        spaceLen += 2;
        willMessageLen = strlen(mqttOptions->willMessage);
    }
    if (mqttOptions->willTopic != NULL)
    {
        spaceLen += 2;
        willTopicLen = strlen(mqttOptions->willTopic);
    }

    currLen = BUFFER_length(ctrlPacket);
    totalLen = clientLen + usernameLen + passwordLen + willMessageLen + willTopicLen + spaceLen;

    // Validate the Username & Password
    if (clientLen > USHRT_MAX)
    {
        result = __FAILURE__;
    }
    else if (usernameLen == 0 && passwordLen > 0)
    {
        result = __FAILURE__;
    }
    else if ((willMessageLen > 0 && willTopicLen == 0) || (willTopicLen > 0 && willMessageLen == 0))
    {
        result = __FAILURE__;
    }
    else if (BUFFER_enlarge(ctrlPacket, totalLen) != 0)
    {
        result = __FAILURE__;
    }
    else
    {
        uint8_t* packet = BUFFER_u_char(ctrlPacket);
        uint8_t* iterator = packet;

        iterator += currLen;
        byteutil_writeUTF(&iterator, mqttOptions->clientId, (uint16_t)clientLen);

        // TODO: Read on the Will Topic
        if (willMessageLen > USHRT_MAX || willTopicLen > USHRT_MAX || usernameLen > USHRT_MAX || passwordLen > USHRT_MAX)
        {
            result = __FAILURE__;
        }
        else
        {
            if (willMessageLen > 0 && willTopicLen > 0)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= WILL_FLAG_FLAG;
                byteutil_writeUTF(&iterator, mqttOptions->willTopic, (uint16_t)willTopicLen);
                packet[CONN_FLAG_BYTE_OFFSET] |= mqttOptions->qualityOfServiceValue;
                if (mqttOptions->messageRetain)
                {
                    packet[CONN_FLAG_BYTE_OFFSET] |= WILL_RETAIN_FLAG;
                }
                byteutil_writeUTF(&iterator, mqttOptions->willMessage, (uint16_t)willMessageLen);
            }
            if (usernameLen > 0)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= USERNAME_FLAG;
                byteutil_writeUTF(&iterator, mqttOptions->username, (uint16_t)usernameLen);
            }
            if (passwordLen > 0)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= PASSWORD_FLAG;
                byteutil_writeUTF(&iterator, mqttOptions->password, (uint16_t)passwordLen);
            }
            // TODO: Get the rest of the flags
            if (mqttOptions->useCleanSession)
            {
                packet[CONN_FLAG_BYTE_OFFSET] |= CLEAN_SESSION_FLAG;
            }
            result = 0;
        }
    }
    return result;
}

static int prepareheaderDataInfo(MQTTCODEC_INSTANCE* codecData, uint8_t remainLen)
{
    int result = 0;
    if (codecData->remainLenIndex >= sizeof(codecData->storeRemainLen))
	    return __FAILURE__;

        codecData->storeRemainLen[codecData->remainLenIndex++] = remainLen;
        if (remainLen < 0x7f)
        {
            int multiplier = 1;
            int totalLen = 0;
            size_t index = 0;
            uint8_t encodeByte = 0;
            do
            {
                encodeByte = codecData->storeRemainLen[index++];
                totalLen += (encodeByte & 0x7f) * multiplier;
                multiplier *= NEXT_128_CHUNK;

                if (multiplier > 128 * 128 * 128)
                {
                    result = __FAILURE__;
                    break;
                }
            } while ((encodeByte & NEXT_128_CHUNK) != 0);

            if (totalLen > 0)
            {
                //codecData->headerData = BUFFER_new();
                (void)BUFFER_pre_build(&codecData->headerData, totalLen);
                codecData->bufferOffset = 0;
            }
            codecData->codecState = CODEC_STATE_VAR_HEADER;

            // Reset remainLen Index
            codecData->remainLenIndex = 0;
            memset(codecData->storeRemainLen, 0, 4 * sizeof(uint8_t));
        }
    return result;
}

static void completePacketData(MQTTCODEC_INSTANCE* codecData)
{
	size_t len = BUFFER_length(&codecData->headerData);
	uint8_t* iterator = BUFFER_u_char(&codecData->headerData);

	switch (codecData->currPacket)
	{
	case CONNACK_TYPE:
	{
		/*Codes_SRS_MQTT_CLIENT_07_028: [If the actionResult parameter is of type CONNECT_ACK then the msgInfo value shall be a CONNECT_ACK structure.]*/
		CONNECT_ACK connack = { 0 };
		connack.isSessionPresent = (byteutil_readByte(&iterator) == 0x1) ? true : false;
		connack.returnCode = byteutil_readByte(&iterator);

		codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, &connack);
		break;
	}
	case PUBLISH_TYPE:
	{
		uint8_t* initialPos = iterator;
		PUBLISH_MSG msg = { 0 };
		msg.isDuplicate = (codecData->headerFlags & PUBLISH_DUP_FLAG) ? true : false;
		msg.isRetained = (codecData->headerFlags & PUBLISH_QOS_RETAIN) ? true : false;
		msg.qosInfo = (codecData->headerFlags == 0) ? DELIVER_AT_MOST_ONCE : (codecData->headerFlags & PUBLISH_QOS_AT_LEAST_ONCE) ? DELIVER_AT_LEAST_ONCE : DELIVER_EXACTLY_ONCE;

		msg.topicName = byteutil_readUTF(&iterator, &msg.topicLen);
		if (msg.topicName == NULL)
		{
			codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, NULL);
			break;
		}

		msg.packetId = 0;
		if (msg.qosInfo != DELIVER_AT_MOST_ONCE)
		{
			msg.packetId = byteutil_read_uint16(&iterator, len - (iterator - initialPos));
		}
		msg.appPayload.message = iterator;
		msg.appPayload.length = len - (iterator - initialPos);

		codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, &msg);
		free(msg.topicName);
		break;
	}
	case PUBACK_TYPE:
	case PUBREC_TYPE:
	case PUBREL_TYPE:
	case PUBCOMP_TYPE:
	{
		/*Codes_SRS_MQTT_CLIENT_07_029: [If the actionResult parameter are of types PUBACK_TYPE, PUBREC_TYPE, PUBREL_TYPE or PUBCOMP_TYPE then the msgInfo value shall be a PUBLISH_ACK structure.]*/
		PUBLISH_ACK publish_ack = { 0 };
		publish_ack.packetId = byteutil_read_uint16(&iterator, len);
		codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, &publish_ack);
		break;
	}
	case SUBACK_TYPE:
	{
		/*Codes_SRS_MQTT_CLIENT_07_030: [If the actionResult parameter is of type SUBACK_TYPE then the msgInfo value shall be a SUBSCRIBE_ACK structure.]*/
		SUBSCRIBE_ACK suback = { 0 };
		suback.packetId = byteutil_read_uint16(&iterator, len);

		// Allocate the remaining len
		suback.qosReturn = iterator;
		suback.qosCount = len-2;
		codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, &suback);
		break;
	}
	case UNSUBACK_TYPE:
	{
		/*Codes_SRS_MQTT_CLIENT_07_031: [If the actionResult parameter is of type UNSUBACK_TYPE then the msgInfo value shall be a UNSUBSCRIBE_ACK structure.]*/
		UNSUBSCRIBE_ACK unsuback = { 0 };
		iterator += VARIABLE_HEADER_OFFSET;
		unsuback.packetId = byteutil_read_uint16(&iterator, len);

		codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, &unsuback);
		break;
	}
	case PINGRESP_TYPE:
		//mqtt_client->timeSincePing = 0;
		// Ping responses do not get forwarded
	default:
		codecData->packetComplete(codecData->callContext, codecData->currPacket, codecData->headerFlags, &codecData->headerData, NULL);
		break;
	}

	// Clean up data
	codecData->currPacket = UNKNOWN_TYPE;
	codecData->codecState = CODEC_STATE_FIXED_HEADER;
	codecData->headerFlags = 0;
	BUFFER_unbuild(&codecData->headerData);
}

void mqtt_codec_init(MQTTCODEC_INSTANCE *result, ON_PACKET_COMPLETE_CALLBACK packetComplete, void* callbackCtx)
{
        /* Codes_SRS_MQTT_CODEC_07_002: [On success mqtt_codec_create shall return a MQTTCODEC_HANDLE value.] */
        result->currPacket = UNKNOWN_TYPE;
        result->codecState = CODEC_STATE_FIXED_HEADER;
        result->headerFlags = 0;
        result->bufferOffset = 0;
        result->packetComplete = packetComplete;
        result->callContext = callbackCtx;
        memset(&result->headerData, 0, sizeof(result->headerData));
        memset(result->storeRemainLen, 0, 4 * sizeof(uint8_t));
        result->remainLenIndex = 0;
}

void mqtt_codec_exit(MQTTCODEC_INSTANCE *codecData)
{
	/* Codes_SRS_MQTT_CODEC_07_004: [mqtt_codec_destroy shall deallocate all memory that has been allocated by this object.] */
	BUFFER_unbuild(&codecData->headerData);
	mqtt_codec_init(codecData, codecData->packetComplete, codecData->callContext);
}

MQTT_BUFFER* mqtt_codec_connect(MQTT_BUFFER* result/*!=NULL*/, const MQTT_CLIENT_OPTIONS* mqttOptions/*!=NULL*/)
{
    // Add Variable Header Information
    if ((constructConnectVariableHeader(result, mqttOptions) != 0)
     || (constructConnPayload(result, mqttOptions) != 0)
     || (constructFixedHeader(result, CONNECT_TYPE, 0) != 0))
    {
        /* Codes_SRS_MQTT_CODEC_07_010: [If any error is encountered then mqtt_codec_connect shall return NULL.] */
        BUFFER_unbuild(result);
        result = NULL;
    }
    return result;
}

MQTT_BUFFER* mqtt_codec_disconnect(MQTT_BUFFER* result/*!=NULL*/)
{
    /* Codes_SRS_MQTT_CODEC_07_011: [On success mqtt_codec_disconnect shall construct a MQTT_BUFFER* that represents a MQTT DISCONNECT packet.] */
    if (result == NULL)
	result = BUFFER_new();
    if (result != NULL)
    {
        if (BUFFER_enlarge(result, 2) != 0)
        {
            /* Codes_SRS_MQTT_CODEC_07_012: [If any error is encountered mqtt_codec_disconnect shall return NULL.] */
            BUFFER_delete(result);
            result = NULL;
        }
        else
        {
            uint8_t* iterator = BUFFER_u_char(result);
            if (iterator == NULL)
            {
                /* Codes_SRS_MQTT_CODEC_07_012: [If any error is encountered mqtt_codec_disconnect shall return NULL.] */
                BUFFER_delete(result);
                result = NULL;
            }
            else
            {
                iterator[0] = DISCONNECT_TYPE;
                iterator[1] = 0;
            }
        }
    }
    return result;
}

MQTT_BUFFER* mqtt_codec_publish(MQTT_BUFFER* result/*!=NULL*/, const PUBLISH_MSG *msg/*!=NULL*/)
{
	uint8_t headerFlags;
	size_t payloadOffset;

    /* Codes_SRS_MQTT_CODEC_07_005: [If the parameters topicName is NULL then mqtt_codec_publish shall return NULL.] */
    if (msg->topicName == NULL)
        return NULL;

    /* Codes_SRS_MQTT_CODEC_07_036: [mqtt_codec_publish shall return NULL if the buffLen variable is greater than the MAX_SEND_SIZE (0xFFFFFF7F).] */
    if (msg->appPayload.length > MAX_SEND_SIZE)
        return NULL;

    headerFlags = 0;
    if (msg->isDuplicate) headerFlags |= PUBLISH_DUP_FLAG;
    if (msg->isRetained) headerFlags |= PUBLISH_QOS_RETAIN;
    if (msg->qosInfo != DELIVER_AT_MOST_ONCE)
    {
        if (msg->qosInfo == DELIVER_AT_LEAST_ONCE)
            headerFlags |= PUBLISH_QOS_AT_LEAST_ONCE;
        else
            headerFlags |= PUBLISH_QOS_EXACTLY_ONCE;
    }

    /* Codes_SRS_MQTT_CODEC_07_007: [mqtt_codec_publish shall return a MQTT_BUFFER* that represents a MQTT PUBLISH message.] */
    if (constructPublishVariableHeader(result, msg) != 0)
    {
        /* Codes_SRS_MQTT_CODEC_07_006: [If any error is encountered then mqtt_codec_publish shall return NULL.] */
        BUFFER_unbuild(result);
        return NULL;
    }

    payloadOffset = BUFFER_length(result);
    if (msg->appPayload.length > 0)
    {
        if (BUFFER_enlarge(result, msg->appPayload.length) != 0)
        {
            /* Codes_SRS_MQTT_CODEC_07_006: [If any error is encountered then mqtt_codec_publish shall return NULL.] */
            BUFFER_unbuild(result);
            return NULL;
        }

        // Write Message
        memcpy(BUFFER_u_char(result)+payloadOffset, msg->appPayload.message, msg->appPayload.length);
    }

    if (constructFixedHeader(result, PUBLISH_TYPE, headerFlags) != 0)
    {
        /* Codes_SRS_MQTT_CODEC_07_006: [If any error is encountered then mqtt_codec_publish shall return NULL.] */
        BUFFER_unbuild(result);
        return NULL;
    }
    return result;
}

MQTT_BUFFER* mqtt_codec_publishAck(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId)
{
    /* Codes_SRS_MQTT_CODEC_07_013: [On success mqtt_codec_publishAck shall return a MQTT_BUFFER* representation of a MQTT PUBACK packet.] */
    /* Codes_SRS_MQTT_CODEC_07_014 : [If any error is encountered then mqtt_codec_publishAck shall return NULL.] */
    return constructPublishReply(result, PUBACK_TYPE, 0, packetId);
}

MQTT_BUFFER* mqtt_codec_publishReceived(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId)
{
    /* Codes_SRS_MQTT_CODEC_07_015: [On success mqtt_codec_publishRecieved shall return a MQTT_BUFFER* representation of a MQTT PUBREC packet.] */
    /* Codes_SRS_MQTT_CODEC_07_016 : [If any error is encountered then mqtt_codec_publishRecieved shall return NULL.] */
    return constructPublishReply(result, PUBREC_TYPE, 0, packetId);
}

MQTT_BUFFER* mqtt_codec_publishRelease(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId)
{
    /* Codes_SRS_MQTT_CODEC_07_017: [On success mqtt_codec_publishRelease shall return a MQTT_BUFFER* representation of a MQTT PUBREL packet.] */
    /* Codes_SRS_MQTT_CODEC_07_018 : [If any error is encountered then mqtt_codec_publishRelease shall return NULL.] */
    return constructPublishReply(result, PUBREL_TYPE, 2, packetId);
}

MQTT_BUFFER* mqtt_codec_publishComplete(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId)
{
    /* Codes_SRS_MQTT_CODEC_07_019: [On success mqtt_codec_publishComplete shall return a MQTT_BUFFER* representation of a MQTT PUBCOMP packet.] */
    /* Codes_SRS_MQTT_CODEC_07_020 : [If any error is encountered then mqtt_codec_publishComplete shall return NULL.] */
    return constructPublishReply(result, PUBCOMP_TYPE, 0, packetId);
}

MQTT_BUFFER* mqtt_codec_ping(MQTT_BUFFER* result/*!=NULL*/)
{
    /* Codes_SRS_MQTT_CODEC_07_021: [On success mqtt_codec_ping shall construct a MQTT_BUFFER* that represents a MQTT PINGREQ packet.] */
    if (BUFFER_enlarge(result, 2) != 0)
    {
        /* Codes_SRS_MQTT_CODEC_07_022: [If any error is encountered mqtt_codec_ping shall return NULL.] */
        BUFFER_unbuild(result);
        result = NULL;
    }
    else
    {
        uint8_t* iterator = BUFFER_u_char(result);
        if (iterator == NULL)
        {
            /* Codes_SRS_MQTT_CODEC_07_022: [If any error is encountered mqtt_codec_ping shall return NULL.] */
            BUFFER_unbuild(result);
            result = NULL;
        }
        else
        {
            iterator[0] = PINGREQ_TYPE;
            iterator[1] = 0;
        }
    }
    return result;
}

MQTT_BUFFER* mqtt_codec_subscribe(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId, SUBSCRIBE_PAYLOAD* subscribeList, size_t count)
{
    /* Codes_SRS_MQTT_CODEC_07_023: [If the parameters subscribeList is NULL or if count is 0 then mqtt_codec_subscribe shall return NULL.] */
    if (subscribeList == NULL || count == 0)
    {
        result = NULL;
    }
    else
    {
            if (constructSubscibeTypeVariableHeader(result, packetId) != 0)
            {
                /* Codes_SRS_MQTT_CODEC_07_025: [If any error is encountered then mqtt_codec_subscribe shall return NULL.] */
                BUFFER_unbuild(result);
                result = NULL;
            }
            else
            {
                /* Codes_SRS_MQTT_CODEC_07_024: [mqtt_codec_subscribe shall iterate through count items in the subscribeList.] */
                if (addListItemsToSubscribePacket(result, subscribeList, count) != 0)
                {
                    /* Codes_SRS_MQTT_CODEC_07_025: [If any error is encountered then mqtt_codec_subscribe shall return NULL.] */
                    BUFFER_unbuild(result);
                    result = NULL;
                }
                else
                {
                    if (constructFixedHeader(result, SUBSCRIBE_TYPE, SUBSCRIBE_FIXED_HEADER_FLAG) != 0)
                    {
                        /* Codes_SRS_MQTT_CODEC_07_025: [If any error is encountered then mqtt_codec_subscribe shall return NULL.] */
                        BUFFER_unbuild(result);
                        result = NULL;
                    }
                }
            }
    }
    return result;
}

MQTT_BUFFER* mqtt_codec_unsubscribe(MQTT_BUFFER *result/*!=NULL*/, uint16_t packetId, const char** unsubscribeList, size_t count)
{
    /* Codes_SRS_MQTT_CODEC_07_027: [If the parameters unsubscribeList is NULL or if count is 0 then mqtt_codec_unsubscribe shall return NULL.] */
    if (unsubscribeList == NULL || count == 0)
    {
        result = NULL;
    }
    else
    {
            if (constructSubscibeTypeVariableHeader(result, packetId) != 0)
            {
                /* Codes_SRS_MQTT_CODEC_07_029: [If any error is encountered then mqtt_codec_unsubscribe shall return NULL.] */
                BUFFER_unbuild(result);
                result = NULL;
            }
            else
            {
                /* Codes_SRS_MQTT_CODEC_07_028: [mqtt_codec_unsubscribe shall iterate through count items in the unsubscribeList.] */
                if (addListItemsToUnsubscribePacket(result, unsubscribeList, count) != 0)
                {
                    /* Codes_SRS_MQTT_CODEC_07_029: [If any error is encountered then mqtt_codec_unsubscribe shall return NULL.] */
                    BUFFER_unbuild(result);
                    result = NULL;
                }
                else
                {
                    if (constructFixedHeader(result, UNSUBSCRIBE_TYPE, UNSUBSCRIBE_FIXED_HEADER_FLAG) != 0)
                    {
                        /* Codes_SRS_MQTT_CODEC_07_029: [If any error is encountered then mqtt_codec_unsubscribe shall return NULL.] */
                        BUFFER_unbuild(result);
                        result = NULL;
                    }
                }
            }
    }
    return result;
}

int mqtt_codec_bytesReceived(MQTTCODEC_INSTANCE* codec_Data, const unsigned char* buffer, size_t size)
{
    int result;
        /* Codes_SRS_MQTT_CODEC_07_033: [mqtt_codec_bytesReceived constructs a sequence of bytes into the corresponding MQTT packets and on success returns zero.] */
        size_t index = 0;
        result = 0;
        for (index = 0; index < size && result == 0; index++)
        {
            uint8_t iterator = ((uint8_t*)buffer)[index];
            if (codec_Data->codecState == CODEC_STATE_FIXED_HEADER)
            {
                if (codec_Data->currPacket == UNKNOWN_TYPE)
                {
                    codec_Data->currPacket = processControlPacketType(iterator, &codec_Data->headerFlags);
                }
                else
                {
                    if (prepareheaderDataInfo(codec_Data, iterator) != 0)
                    {
                        /* Codes_SRS_MQTT_CODEC_07_035: [If any error is encountered then the packet state will be marked as error and mqtt_codec_bytesReceived shall return a non-zero value.] */
                        codec_Data->currPacket = PACKET_TYPE_ERROR;
                        result = __FAILURE__;
                    }
                    if (codec_Data->currPacket == PINGRESP_TYPE)
                    {
                        /* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
                        completePacketData(codec_Data);
                    }
                }
            }
            else if (codec_Data->codecState == CODEC_STATE_VAR_HEADER)
            {
                    uint8_t* dataBytes = BUFFER_u_char(&codec_Data->headerData);
                    if (dataBytes == NULL)
                    {
                        /* Codes_SRS_MQTT_CODEC_07_035: [If any error is encountered then the packet state will be marked as error and mqtt_codec_bytesReceived shall return a non-zero value.] */
                        codec_Data->currPacket = PACKET_TYPE_ERROR;
                        result = __FAILURE__;
                    }
                    else
                    {
                        size_t totalLen = BUFFER_length(&codec_Data->headerData);
                        // Increment the data
                        dataBytes += codec_Data->bufferOffset++;
                        *dataBytes = iterator;

                        if (codec_Data->bufferOffset >= totalLen)
                        {
                            /* Codes_SRS_MQTT_CODEC_07_034: [Upon a constructing a complete MQTT packet mqtt_codec_bytesReceived shall call the ON_PACKET_COMPLETE_CALLBACK function.] */
                            completePacketData(codec_Data);
                        }
                    }
            }
            else
            {
                /* Codes_SRS_MQTT_CODEC_07_035: [If any error is encountered then the packet state will be marked as error and mqtt_codec_bytesReceived shall return a non-zero value.] */
                codec_Data->currPacket = PACKET_TYPE_ERROR;
                result = __FAILURE__;
            }
        }
    return result;
}
