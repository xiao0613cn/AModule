#ifndef _AMODULE_RTMP_H_
#define _AMODULE_RTMP_H_


#define RTMP_HANDSHAKE_PACKET_SIZE 1536

typedef struct RTMPCtx {
	int     in_chunk_size;  ///< size of the chunks incoming RTMP packets are divided into
	int     out_chunk_size; ///< size of the chunks outgoing RTMP packets are divided into
	int     encrypted;      ///< use an encrypted connection (RTMPE only)
	int     is_input;       ///< input/output flag
	char    flashver[64];   ///< version of the flash plugin
	int     flashver_len;
	unsigned char swfhash[32]; ///< SHA256 hash of the decompressed SWF file (32 bytes)
	int     swfhash_len;    ///< length of the SHA256 hash
	int     swfsize;        ///< size of the decompressed SWF file
//	char*   swfverify;      ///< URL to player swf file, compute hash/size automatically
	char    swfverification[42]; ///< hash of the SWF verification

	char*   swfurl;         ///< url of the swf player
	char*   pageurl;        ///< url of the web page

	int     c0c1_pos;
	int     nb_invokes;

	// rtmpe
	struct FF_DH *dh;
} RTMPCtx;

AMODULE_API void
rtmp_init(RTMPCtx *rt, int is_input);

AMODULE_API int
rtmp_gen_c0c1(RTMPCtx *rt, unsigned char c0c1[1+RTMP_HANDSHAKE_PACKET_SIZE]);

AMODULE_API int
rtmp_gen_c2(RTMPCtx *rt, unsigned char c0c1c2[1+RTMP_HANDSHAKE_PACKET_SIZE],
            const unsigned char s0s1[1+RTMP_HANDSHAKE_PACKET_SIZE],
            const unsigned char s2[RTMP_HANDSHAKE_PACKET_SIZE]);

AMODULE_API int
rtmp_gen_s0s1s2(RTMPCtx *rt, unsigned char s0s1[1+RTMP_HANDSHAKE_PACKET_SIZE],
                unsigned char s2[RTMP_HANDSHAKE_PACKET_SIZE],
                const unsigned char c0c1[1+RTMP_HANDSHAKE_PACKET_SIZE]);

AMODULE_API int
rtmp_check_c2(RTMPCtx *rt, const unsigned char c2[RTMP_HANDSHAKE_PACKET_SIZE],
              const unsigned char s2[RTMP_HANDSHAKE_PACKET_SIZE]);

AMODULE_API int
rtmp_calc_swfhash(RTMPCtx *rt, const unsigned char *swfdata, int swfsize);

AMODULE_API int
rtmp_gen_connect(RTMPCtx *rt, unsigned char *data, const char **app, const char **tcurl, char *param);

//////////////////////////////////////////////////////////////////////////
/** maximum possible number of different RTMP channels */
#define RTMP_CHANNELS 65599

/**
 * channels used to for RTMP packets with different purposes (i.e. data, network
 * control, remote procedure calls, etc.)
 */
enum RTMPChannel { // RTMP chunk basic header: chunk stream id
    RTMP_NETWORK_CHANNEL = 2,   ///< channel for network-related messages (bandwidth report, ping, etc)
    RTMP_SYSTEM_CHANNEL,        ///< channel for sending server control messages
    RTMP_AUDIO_CHANNEL,         ///< channel for audio data
    RTMP_VIDEO_CHANNEL   = 6,   ///< channel for video data
    RTMP_SOURCE_CHANNEL  = 8,   ///< channel for a/v invokes
};

/**
 * known RTMP packet types
 */
typedef enum RTMPPacketType {
    RTMP_PT_CHUNK_SIZE   =  1,  ///< chunk size change, payload: chunk size(0+31 bits)
    RTMP_PT_ABORT        =  2,  ///< abort message, payload: chunk stream id(32 bits)
    RTMP_PT_BYTES_READ   =  3,  ///< number of bytes read: 4 bytes
    RTMP_PT_PING,               ///< ping
    RTMP_PT_SERVER_BW,          ///< server bandwidth: 4 bytes
    RTMP_PT_CLIENT_BW,          ///< client bandwidth: 4+1 bytes
    RTMP_PT_AUDIO        =  8,  ///< audio packet
    RTMP_PT_VIDEO,              ///< video packet
    RTMP_PT_FLEX_STREAM  = 15,  ///< Flex shared stream
    RTMP_PT_FLEX_OBJECT,        ///< Flex shared object
    RTMP_PT_FLEX_MESSAGE,       ///< Flex shared message
    RTMP_PT_NOTIFY,             ///< some notification
    RTMP_PT_SHARED_OBJ,         ///< shared object
    RTMP_PT_INVOKE,             ///< invoke some stream action
    RTMP_PT_METADATA     = 22,  ///< FLV metadata
} RTMPPacketType;

/**
 * possible RTMP packet header sizes
 */
enum RTMPPacketSize { // RTMP chunk basic header: chunk packet/message format
    RTMP_PS_TWELVEBYTES = 0, ///< packet has 12-byte header
    RTMP_PS_EIGHTBYTES,      ///< packet has 8-byte header
    RTMP_PS_FOURBYTES,       ///< packet has 4-byte header
    RTMP_PS_ONEBYTE          ///< packet is really a next chunk of a packet
};

/**
 * structure for holding RTMP packets
 */
typedef struct RTMPPacket {
    int            channel_id; ///< RTMP channel ID (nothing to do with audio/video channels though)
    RTMPPacketType type;       ///< packet payload type
    uint32_t       timestamp;  ///< packet full timestamp
    uint32_t       ts_field;   ///< 24-bit timestamp or increment to the previous one, in milliseconds (latter only for media packets). Clipped to a maximum of 0xFFFFFF, indicating an extended timestamp field.
    uint32_t       extra;      ///< probably an additional channel ID used during streaming data
    uint8_t        *data;      ///< packet payload
    int            size;       ///< packet payload size
    int            offset;     ///< amount of data read so far
    int            read;       ///< amount read, including headers
} RTMPPacket;

AMODULE_API int
rtmp_parse_one_chunk(RTMPCtx *rt, RTMPPacket *p, RTMPPacket *prev_pkt);

AMODULE_API int
rtmp_gen_chunk_head(RTMPCtx *rt, unsigned char data[16], RTMPPacket *pkt, RTMPPacket *prev_pkt);
//////////////////////////////////////////////////////////////////////////

#endif
