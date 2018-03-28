// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
//#include "azure_c_shared_utility/gballoc.h"
#include "buffer_.h"
typedef MQTT_BUFFER BUFFER;
//#include "azure_c_shared_utility/optimize_size.h"
//#include "azure_c_shared_utility/xlogging.h"


/* Codes_SRS_BUFFER_07_001: [BUFFER_new shall allocate a MQTT_BUFFER* that will contain a NULL unsigned char*.] */
MQTT_BUFFER* BUFFER_new(void)
{
    BUFFER* temp = (BUFFER*)malloc(sizeof(BUFFER));
    /* Codes_SRS_BUFFER_07_002: [BUFFER_new shall return NULL on any error that occurs.] */
    if (temp != NULL)
    {
        temp->buffer = NULL;
        temp->size = 0;
    }
    return (MQTT_BUFFER*)temp;
}

static int BUFFER_safemalloc(BUFFER* handleptr, size_t size)
{
    int result;
    size_t sizetomalloc = size;
    if (size == 0)
    {
        sizetomalloc = 1;
    }
    handleptr->buffer = (unsigned char*)malloc(sizetomalloc);
    if (handleptr->buffer == NULL)
    {
        /*Codes_SRS_BUFFER_02_003: [If allocating memory fails, then BUFFER_create shall return NULL.]*/
        result = __FAILURE__;
    }
    else
    {
        // we still consider the real buffer size is 0
        handleptr->size = size;
        result = 0;
    }
    return result;
}

MQTT_BUFFER* BUFFER_create(const unsigned char* source, size_t size)
{
    BUFFER* result;
    /*Codes_SRS_BUFFER_02_001: [If source is NULL then BUFFER_create shall return NULL.]*/
    if (source == NULL)
    {
        result = NULL;
    }
    else
    {
        /*Codes_SRS_BUFFER_02_002: [Otherwise, BUFFER_create shall allocate memory to hold size bytes and shall copy from source size bytes into the newly allocated memory.] */
        result = (BUFFER*)malloc(sizeof(BUFFER));
        if (result == NULL)
        {
            /*Codes_SRS_BUFFER_02_003: [If allocating memory fails, then BUFFER_create shall return NULL.] */
            /*fallthrough*/
        }
        else
        {
            /* Codes_SRS_BUFFER_02_005: [If size parameter is 0 then 1 byte of memory shall be allocated yet size of the buffer shall be set to 0.]*/
            if (BUFFER_safemalloc(result, size) != 0)
            {
                free(result);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_BUFFER_02_004: [Otherwise, BUFFER_create shall return a non-NULL handle.] */
                (void)memcpy(result->buffer, source, size);
            }
        }
    }
    return (MQTT_BUFFER*)result;
}

/* Codes_SRS_BUFFER_07_003: [BUFFER_delete shall delete the data associated with the MQTT_BUFFER* along with the Buffer.] */
void BUFFER_delete(MQTT_BUFFER* handle)
{
    /* Codes_SRS_BUFFER_07_004: [BUFFER_delete shall not delete any MQTT_BUFFER* that is NULL.] */
    if (handle != NULL)
    {
        BUFFER* b = (BUFFER*)handle;
        if (b->buffer != NULL)
        {
            /* Codes_SRS_BUFFER_07_003: [BUFFER_delete shall delete the data associated with the MQTT_BUFFER* along with the Buffer.] */
            free(b->buffer);
        }
        free(b);
    }
}

/*return 0 if the buffer was copied*/
/*else return different than zero*/
/* Codes_SRS_BUFFER_07_008: [BUFFER_build allocates size_t bytes, copies the unsigned char* into the buffer and returns zero on success.] */
int BUFFER_build(MQTT_BUFFER* b, const unsigned char* source, size_t size)
{
    int result;
    /* Codes_SRS_BUFFER_01_002: [The size argument can be zero, in which case the underlying buffer held by the buffer instance shall be freed.] */
    if (size == 0)
    {
        /* Codes_SRS_BUFFER_01_003: [If size is zero, source can be NULL.] */
        if (b->buffer != NULL) free(b->buffer);
        b->buffer = NULL;
        b->size = 0;

        result = 0;
    }
    else
    {
        /* Codes_SRS_BUFFER_07_011: [BUFFER_build shall overwrite previous contents if the buffer has been previously allocated.] */
        unsigned char* newBuffer = (unsigned char*)realloc(b->buffer, size);
        if (newBuffer == NULL)
        {
            /* Codes_SRS_BUFFER_07_010: [BUFFER_build shall return nonzero if any error is encountered.] */
            result = __FAILURE__;
        }
        else
        {
            b->buffer = newBuffer;
            b->size = size;
            /* Codes_SRS_BUFFER_01_002: [The size argument can be zero, in which case nothing shall be copied from source.] */
            memcpy(b->buffer, source, size);

            result = 0;
        }
    }

    return result;
}

/*return 0 if the buffer was pre-build(that is, had its space allocated)*/
/*else return different than zero*/
/* Codes_SRS_BUFFER_07_005: [BUFFER_pre_build allocates size_t bytes of MQTT_BUFFER* and returns zero on success.] */
int BUFFER_pre_build(MQTT_BUFFER* handle, size_t size)
{
    int result;
        BUFFER* b = (BUFFER*)handle;
        if (b->buffer != NULL)
        {
            /* Codes_SRS_BUFFER_07_007: [BUFFER_pre_build shall return nonzero if the buffer has been previously allocated and is not NULL.] */
            result = __FAILURE__;
        }
        else
        {
            if ((b->buffer = (unsigned char*)malloc(size ? size : 1)) == NULL)
            {
                /* Codes_SRS_BUFFER_07_013: [BUFFER_pre_build shall return nonzero if any error is encountered.] */
                result = __FAILURE__;
            }
            else
            {
                b->size = size;
                result = 0;
            }
        }
    return result;
}

/* Codes_SRS_BUFFER_07_019: [BUFFER_content shall return the data contained within the MQTT_BUFFER*.] */
int BUFFER_content(MQTT_BUFFER* handle, const unsigned char** content)
{
    int result;
    if ((handle == NULL) || (content == NULL))
    {
        /* Codes_SRS_BUFFER_07_020: [If the handle and/or content*is NULL BUFFER_content shall return nonzero.] */
        result = __FAILURE__;
    }
    else
    {
        BUFFER* b = (BUFFER*)handle;
        *content = b->buffer;
        result = 0;
    }
    return result;
}

/*return 0 if everything went ok and whatever was built in the buffer was unbuilt*/
/* Codes_SRS_BUFFER_07_012: [BUFFER_unbuild shall clear the underlying unsigned char* data associated with the MQTT_BUFFER* this will return zero on success.] */
extern int BUFFER_unbuild(MQTT_BUFFER* handle)
{
    int result;
        BUFFER* b = (BUFFER*)handle;
        if (b->buffer != NULL)
        {
            free(b->buffer);
            b->buffer = NULL;
            b->size = 0;
            result = 0;
        }
        else
        {
            /* Codes_SRS_BUFFER_07_015: [BUFFER_unbuild shall return a nonzero value if the unsigned char* referenced by MQTT_BUFFER* is NULL.] */
            result = __FAILURE__;
        }
    return result;
}

/* Codes_SRS_BUFFER_07_016: [BUFFER_enlarge shall increase the size of the unsigned char* referenced by MQTT_BUFFER*.] */
int BUFFER_enlarge(MQTT_BUFFER* handle, size_t enlargeSize)
{
    int result;
    if (enlargeSize == 0)
    {
        /* Codes_SRS_BUFFER_07_017: [BUFFER_enlarge shall return a nonzero result if any parameters are NULL or zero.] */
        result = 0; // __FAILURE__;
    }
    else
    {
        BUFFER* b = (BUFFER*)handle;
        unsigned char* temp = (unsigned char*)realloc(b->buffer, b->size + enlargeSize);
        if (temp == NULL)
        {
            /* Codes_SRS_BUFFER_07_018: [BUFFER_enlarge shall return a nonzero result if any error is encountered.] */
            result = __FAILURE__;
        }
        else
        {
            b->buffer = temp;
            b->size += enlargeSize;
            result = 0;
        }
    }
    return result;
}

/* Codes_SRS_BUFFER_07_021: [BUFFER_size shall place the size of the associated buffer in the size variable and return zero on success.] */
int BUFFER_size(MQTT_BUFFER* handle, size_t* size)
{
    int result;
    if ((handle == NULL) || (size == NULL))
    {
        /* Codes_SRS_BUFFER_07_022: [BUFFER_size shall return a nonzero value for any error that is encountered.] */
        result = __FAILURE__;
    }
    else
    {
        BUFFER* b = (BUFFER*)handle;
        *size = b->size;
        result = 0;
    }
    return result;
}

/* Codes_SRS_BUFFER_07_024: [BUFFER_append concatenates b2 onto b1 without modifying b2 and shall return zero on success.] */
int BUFFER_append(MQTT_BUFFER* b1, const void *buffer, size_t size)
{
    int result;
    if ((b1 == NULL) || (b1->buffer == buffer))
    {
        /* Codes_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
        result = __FAILURE__;
    }
    else
    {
        if (size ==0)
        {
            // b2->size = 0, whatever b1->size is, do nothing
            result = 0;
        }
        else
        {
            // b2->size != 0, whatever b1->size is
            unsigned char* temp = (unsigned char*)malloc(b1->size + size);
            if (temp == NULL)
            {
                /* Codes_SRS_BUFFER_07_023: [BUFFER_append shall return a nonzero upon any error that is encountered.] */
                result = __FAILURE__;
            }
            else
			{
				/* Codes_SRS_BUFFER_07_024: [BUFFER_append concatenates b2 onto b1 without modifying b2 and shall return zero on success.]*/
				memcpy(temp, b1->buffer, b1->size);
                // Append the BUFFER
				memcpy(&temp[b1->size], buffer, size);

				if (b1->buffer != NULL) free(b1->buffer);
				b1->buffer = temp;
                b1->size += size;
                result = 0;
            }
        }
    }
    return result;
}

int BUFFER_prepend(MQTT_BUFFER* b1, const void *buffer, size_t size)
{
    int result;
    if ((b1 == NULL) || (b1->buffer == buffer))
    {
        /* Codes_SRS_BUFFER_01_005: [ BUFFER_prepend shall return a non-zero upon value any error that is encountered. ]*/
        result = __FAILURE__;
    }
    else
    {
        //put b2 ahead of b1: [b2][b1], return b1
        if (size == 0)
        {
            // do nothing
            result = 0;
        }
        else
        {
            // b2->size != 0
            unsigned char* temp = (unsigned char*)malloc(b1->size + size);
            if (temp == NULL)
            {
                /* Codes_SRS_BUFFER_01_005: [ BUFFER_prepend shall return a non-zero upon value any error that is encountered. ]*/
                result = __FAILURE__;
            }
            else
            {
                /* Codes_SRS_BUFFER_01_004: [ BUFFER_prepend concatenates handle1 onto handle2 without modifying handle1 and shall return zero on success. ]*/
                // Append the BUFFER
                memcpy(temp, buffer, size);
                // start from b1->size to append b1
                memcpy(&temp[size], b1->buffer, b1->size);

				if (b1->buffer != NULL) free(b1->buffer);
                b1->buffer = temp;
                b1->size += size;
                result = 0;
            }
        }
    }
    return result;
}


/* Codes_SRS_BUFFER_07_025: [BUFFER_u_char shall return a pointer to the underlying unsigned char*.] */
unsigned char* BUFFER_u_char(MQTT_BUFFER* handle)
{
    BUFFER* handleData = (BUFFER*)handle;
    unsigned char* result;
    if (handle == NULL || handleData->size == 0)
    {
        /* Codes_SRS_BUFFER_07_026: [BUFFER_u_char shall return NULL for any error that is encountered.] */
        /* Codes_SRS_BUFFER_07_029: [BUFFER_u_char shall return NULL if underlying buffer size is zero.] */
        result = NULL;
    }
    else
    {
        result = handleData->buffer;
    }
    return result;
}

/* Codes_SRS_BUFFER_07_027: [BUFFER_length shall return the size of the underlying buffer.] */
size_t BUFFER_length(MQTT_BUFFER* handle)
{
    size_t result;
    if (handle == NULL)
    {
        /* Codes_SRS_BUFFER_07_028: [BUFFER_length shall return zero for any error that is encountered.] */
        result = 0;
    }
    else
    {
        BUFFER* b = (BUFFER*)handle;
        result = b->size;
    }
    return result;
}

MQTT_BUFFER* BUFFER_clone(MQTT_BUFFER* handle)
{
    MQTT_BUFFER* result;
    if (handle == NULL)
    {
        result = NULL;
    }
    else
    {
        BUFFER* suppliedBuff = (BUFFER*)handle;
        BUFFER* b = (BUFFER*)malloc(sizeof(BUFFER));
        if (b != NULL)
        {
            if (BUFFER_safemalloc(b, suppliedBuff->size) != 0)
            {
                result = NULL;
            }
            else
            {
                (void)memcpy(b->buffer, suppliedBuff->buffer, suppliedBuff->size);
                b->size = suppliedBuff->size;
                result = (MQTT_BUFFER*)b;
            }
        }
        else
        {
            result = NULL;
        }
    }
    return result;
}
