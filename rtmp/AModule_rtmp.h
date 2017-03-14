#ifndef _AMODULE_RTMP_H_
#define _AMODULE_RTMP_H_


#define RTMP_HANDSHAKE_PACKET_SIZE 1536

struct RTMPCtx {
	int     encrypted; ///< use an encrypted connection (RTMPE only)
	int     is_input;  ///< input/output flag
	char*   swfhash;   ///< SHA256 hash of the decompressed SWF file (32 bytes)
	int     swfhash_len; ///< length of the SHA256 hash
	int     swfsize;   ///< size of the decompressed SWF file
	char*   swfurl;    ///< url of the swf player
	char*   swfverify; ///< URL to player swf file, compute hash/size automatically
	char    swfverification[42]; ///< hash of the SWF verification

	int     c0c1_pos;

	// rtmpe
	struct FF_DH *dh;
};


AMODULE_API int
rtmp_gen_c0c1(RTMPCtx *rt, unsigned char c0c1[1+RTMP_HANDSHAKE_PACKET_SIZE]);

AMODULE_API int
rtmp_gen_c2(RTMPCtx *rt, unsigned char c0c1c2[1+RTMP_HANDSHAKE_PACKET_SIZE],
	    const unsigned char s0s1[1+RTMP_HANDSHAKE_PACKET_SIZE],
	    const unsigned char s2[RTMP_HANDSHAKE_PACKET_SIZE]);

#endif
