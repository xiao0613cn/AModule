/*
 * RTMP packet utilities
 * Copyright (c) 2009 Konstantin Shishkov
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_RTMPPKT_H
#define AVFORMAT_RTMPPKT_H

//#include "bytestream.h"
//#include "avformat.h"
//#include "url.h"
#include "AModule_rtmp.h"

#if 0
/**
 * Create new RTMP packet with given attributes.
 *
 * @param pkt        packet
 * @param channel_id packet channel ID
 * @param type       packet type
 * @param timestamp  packet timestamp
 * @param size       packet size
 * @return zero on success, negative value otherwise
 */
int ff_rtmp_packet_create(RTMPPacket *pkt, int channel_id, RTMPPacketType type,
                          int timestamp, int size);

/**
 * Free RTMP packet.
 *
 * @param pkt packet
 */
void ff_rtmp_packet_destroy(RTMPPacket *pkt);

/**
 * Read RTMP packet sent by the server.
 *
 * @param h          reader context
 * @param p          packet
 * @param chunk_size current chunk size
 * @param prev_pkt   previously read packet headers for all channels
 *                   (may be needed for restoring incomplete packet header)
 * @param nb_prev_pkt number of allocated elements in prev_pkt
 * @return number of bytes read on success, negative value otherwise
 */
int ff_rtmp_packet_read(URLContext *h, RTMPPacket *p,
                        int chunk_size, RTMPPacket **prev_pkt,
                        int *nb_prev_pkt);
/**
 * Read internal RTMP packet sent by the server.
 *
 * @param h          reader context
 * @param p          packet
 * @param chunk_size current chunk size
 * @param prev_pkt   previously read packet headers for all channels
 *                   (may be needed for restoring incomplete packet header)
 * @param nb_prev_pkt number of allocated elements in prev_pkt
 * @param c          the first byte already read
 * @return number of bytes read on success, negative value otherwise
 */
int ff_rtmp_packet_read_internal(URLContext *h, RTMPPacket *p, int chunk_size,
                                 RTMPPacket **prev_pkt, int *nb_prev_pkt,
                                 uint8_t c);

/**
 * Send RTMP packet to the server.
 *
 * @param h          reader context
 * @param p          packet to send
 * @param chunk_size current chunk size
 * @param prev_pkt   previously sent packet headers for all channels
 *                   (may be used for packet header compressing)
 * @param nb_prev_pkt number of allocated elements in prev_pkt
 * @return number of bytes written on success, negative value otherwise
 */
int ff_rtmp_packet_write(URLContext *h, RTMPPacket *p,
                         int chunk_size, RTMPPacket **prev_pkt,
                         int *nb_prev_pkt);

/**
 * Print information and contents of RTMP packet.
 *
 * @param ctx        output context
 * @param p          packet to dump
 */
void ff_rtmp_packet_dump(void *ctx, RTMPPacket *p);

/**
 * Enlarge the prev_pkt array to fit the given channel
 *
 * @param prev_pkt    array with previously sent packet headers
 * @param nb_prev_pkt number of allocated elements in prev_pkt
 * @param channel     the channel number that needs to be allocated
 */
int ff_rtmp_check_alloc_array(RTMPPacket **prev_pkt, int *nb_prev_pkt,
                              int channel);
#endif
/**
 * @name Functions used to work with the AMF format (which is also used in .flv)
 * @see amf_* funcs in libavformat/flvdec.c
 * @{
 */

/**
 * Calculate number of bytes taken by first AMF entry in data.
 *
 * @param data input data
 * @param data_end input buffer end
 * @return number of bytes used by first AMF entry
 */
int ff_amf_tag_size(const uint8_t *data, const uint8_t *data_end);

/**
 * Retrieve value of given AMF object field in string form.
 *
 * @param data     AMF object data
 * @param data_end input buffer end
 * @param name     name of field to retrieve
 * @param dst      buffer for storing result
 * @param dst_size output buffer size
 * @return 0 if search and retrieval succeeded, negative value otherwise
 */
int ff_amf_get_field_value(const uint8_t *data, const uint8_t *data_end,
                           const uint8_t *name, uint8_t *dst, int dst_size);

/**
 * Write boolean value in AMF format to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 * @param val value to write
 */
void ff_amf_write_bool(uint8_t **dst, int val);

/**
 * Write number in AMF format to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 * @param num value to write
 */
void ff_amf_write_number(uint8_t **dst, double num);

/**
 * Write string in AMF format to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 * @param str string to write
 */
void ff_amf_write_string(uint8_t **dst, const char *str, int len);
#define ff_amf_write_string_sz(dst, str) ff_amf_write_string(dst, str, sizeof(str)-1)

/**
 * Write a string consisting of two parts in AMF format to a buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 * @param str1 first string to write, may be null
 * @param str2 second string to write, may be null
 */
void ff_amf_write_string2(uint8_t **dst, const char *str1, const char *str2);

/**
 * Write AMF NULL value to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 */
void ff_amf_write_null(uint8_t **dst);

/**
 * Write marker for AMF object to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 */
void ff_amf_write_object_start(uint8_t **dst);

/**
 * Write string used as field name in AMF object to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 * @param str string to write
 */
void ff_amf_write_field_name(uint8_t **dst, const char *str, int len);
#define ff_amf_write_field_name_sz(dst, str) ff_amf_write_field_name(dst, str, sizeof(str)-1)

/**
 * Write marker for end of AMF object to buffer.
 *
 * @param dst pointer to the input buffer (will be modified)
 */
void ff_amf_write_object_end(uint8_t **dst);

/**
 * Read AMF boolean value.
 *
 *@param[in,out] gbc GetByteContext initialized with AMF-formatted data
 *@param[out]    val 0 or 1
 *@return 0 on success or an AVERROR code on failure
*/
int ff_amf_read_bool(GetByteContext *gbc, int *val);

/**
 * Read AMF number value.
 *
 *@param[in,out] gbc GetByteContext initialized with AMF-formatted data
 *@param[out]    val read value
 *@return 0 on success or an AVERROR code on failure
*/
int ff_amf_read_number(GetByteContext *gbc, double *val);

/**
 * Get AMF string value.
 *
 * This function behaves the same as ff_amf_read_string except that
 * it does not expect the AMF type prepended to the actual data.
 * Appends a trailing null byte to output string in order to
 * ease later parsing.
 *
 *@param[in,out] gbc     GetByteContext initialized with AMF-formatted data
 *@param[out]    str     read string
 *@param[in]     strsize buffer size available to store the read string
 *@param[out]    length  read string length
 *@return 0 on success or an AVERROR code on failure
*/
int ff_amf_get_string(GetByteContext *bc, uint8_t *str,
                      int strsize, int *length);

/**
 * Read AMF string value.
 *
 * Appends a trailing null byte to output string in order to
 * ease later parsing.
 *
 *@param[in,out] gbc     GetByteContext initialized with AMF-formatted data
 *@param[out]    str     read string
 *@param[in]     strsize buffer size available to store the read string
 *@param[out]    length  read string length
 *@return 0 on success or an AVERROR code on failure
*/
int ff_amf_read_string(GetByteContext *gbc, uint8_t *str,
                       int strsize, int *length);

/**
 * Read AMF NULL value.
 *
 *@param[in,out] gbc GetByteContext initialized with AMF-formatted data
 *@return 0 on success or an AVERROR code on failure
*/
int ff_amf_read_null(GetByteContext *gbc);

/**
 * Match AMF string with a NULL-terminated string.
 *
 * @return 0 if the strings do not match.
 */

int ff_amf_match_string(const uint8_t *data, int size, const char *str);

/** @} */ // AMF funcs

#endif /* AVFORMAT_RTMPPKT_H */
