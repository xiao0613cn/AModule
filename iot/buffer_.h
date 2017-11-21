// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include "mqttconst.h"

#ifdef __cplusplus
//#include <cstddef>
extern "C" {
#else
#endif

//#include "umock_c_prod.h"

typedef struct MQTT_BUFFER
{
	unsigned char* buffer;
	size_t size;
} MQTT_BUFFER;

MQTT_BUFFER* BUFFER_new();
MQTT_BUFFER* BUFFER_create(const unsigned char* source, size_t size);
void BUFFER_delete(MQTT_BUFFER *handle);
int BUFFER_pre_build(MQTT_BUFFER *handle, size_t size);
int BUFFER_build(MQTT_BUFFER *handle, const unsigned char* source, size_t size);
int BUFFER_unbuild(MQTT_BUFFER *handle);
int BUFFER_enlarge(MQTT_BUFFER *handle, size_t enlargeSize);
int BUFFER_content(MQTT_BUFFER *handle, const unsigned char** content);
int BUFFER_size(MQTT_BUFFER *handle, size_t* size);
int BUFFER_append(MQTT_BUFFER *handle, const void *buffer, size_t size);
int BUFFER_prepend(MQTT_BUFFER *handle, const void *buffer, size_t size);
unsigned char* BUFFER_u_char(MQTT_BUFFER *handle);
size_t BUFFER_length(MQTT_BUFFER *handle);
MQTT_BUFFER* BUFFER_clone(MQTT_BUFFER *handle);

#ifdef __cplusplus
}
#endif


#endif  /* BUFFER_H */
