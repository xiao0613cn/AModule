// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
//#include <cstddef>
extern "C" {
#else
#endif

#include "umock_c_prod.h"

typedef struct MQTT_BUFFER_TAG
{
	unsigned char* buffer;
	size_t size;
} MQTT_BUFFER, * BUFFER_HANDLE;

BUFFER_HANDLE BUFFER_new();
BUFFER_HANDLE BUFFER_create(const unsigned char* source, size_t size);
void BUFFER_delete(BUFFER_HANDLE handle);
int BUFFER_pre_build(BUFFER_HANDLE handle, size_t size);
int BUFFER_build(BUFFER_HANDLE handle, const unsigned char* source, size_t size);
int BUFFER_unbuild(BUFFER_HANDLE handle);
int BUFFER_enlarge(BUFFER_HANDLE handle, size_t enlargeSize);
int BUFFER_content(BUFFER_HANDLE handle, const unsigned char** content);
int BUFFER_size(BUFFER_HANDLE handle, size_t* size);
int BUFFER_append(BUFFER_HANDLE handle1, BUFFER_HANDLE handle2);
int BUFFER_prepend(BUFFER_HANDLE handle1, BUFFER_HANDLE handle2);
unsigned char* BUFFER_u_char(BUFFER_HANDLE handle);
size_t BUFFER_length(BUFFER_HANDLE handle);
BUFFER_HANDLE BUFFER_clone(BUFFER_HANDLE handle);

#ifdef __cplusplus
}
#endif


#endif  /* BUFFER_H */
