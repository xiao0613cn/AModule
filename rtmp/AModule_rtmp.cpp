#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_rtmp.h"
extern "C" {
#include "lfg.h"
#include "sha.h"
#include "md5.h"
#include "rtmpdh.h"
#include "bytestream.h"
};

#define RTMP_CLIENT_PLATFORM "LNX"
#define RTMP_CLIENT_VER1    9
#define RTMP_CLIENT_VER2    0
#define RTMP_CLIENT_VER3  124
#define RTMP_CLIENT_VER4    2

const unsigned char rtmp_c0[] = {
	3,                // unencrypted data
	0, 0, 0, 0,       // client uptime
	RTMP_CLIENT_VER1,
	RTMP_CLIENT_VER2,
	RTMP_CLIENT_VER3,
	RTMP_CLIENT_VER4,
};

int ff_rtmp_calc_digest_pos(const uint8_t *buf, int off, int mod_val,
			    int add_val)
{
	int i, digest_pos = 0;

	for (i = 0; i < 4; i++)
		digest_pos += buf[i + off];
	digest_pos = digest_pos % mod_val + add_val;

	return digest_pos;
}

#define HMAC_IPAD_VAL 0x36
#define HMAC_OPAD_VAL 0x5C

int ff_rtmp_calc_digest(const uint8_t *src, int len, int gap,
			const uint8_t *key, int keylen, uint8_t *dst)
{
	struct AVSHA avsha;
	struct AVSHA *sha = &avsha;
	uint8_t hmac_buf[64+32] = {0};
	int i;

	//sha = av_mallocz(av_sha_size);
	//if (!sha)
	//	return AVERROR(ENOMEM);

	if (keylen < 64) {
		memcpy(hmac_buf, key, keylen);
	} else {
		av_sha_init(sha, 256);
		av_sha_update(sha,key, keylen);
		av_sha_final(sha, hmac_buf);
	}
	for (i = 0; i < 64; i++)
		hmac_buf[i] ^= HMAC_IPAD_VAL;

	av_sha_init(sha, 256);
	av_sha_update(sha, hmac_buf, 64);
	if (gap <= 0) {
		av_sha_update(sha, src, len);
	} else { //skip 32 bytes used for storing digest
		av_sha_update(sha, src, gap);
		av_sha_update(sha, src + gap + 32, len - gap - 32);
	}
	av_sha_final(sha, hmac_buf + 64);

	for (i = 0; i < 64; i++)
		hmac_buf[i] ^= HMAC_IPAD_VAL ^ HMAC_OPAD_VAL; //reuse XORed key for opad
	av_sha_init(sha, 256);
	av_sha_update(sha, hmac_buf, 64+32);
	av_sha_final(sha, dst);

	//av_free(sha);

	return 0;
}

#define PLAYER_KEY_OPEN_PART_LEN 30   ///< length of partial key used for first client digest signing
/** Client key used for digest signing */
static const uint8_t rtmp_player_key[] = {
	'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
	'F', 'l', 'a', 's', 'h', ' ', 'P', 'l', 'a', 'y', 'e', 'r', ' ', '0', '0', '1',

	0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
	0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
	0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};

/**
 * Put HMAC-SHA2 digest of packet data (except for the bytes where this digest
 * will be stored) into that packet.
 *
 * @param buf handshake data (1536 bytes)
 * @param encrypted use an encrypted connection (RTMPE)
 * @return offset to the digest inside input data
 */
static int rtmp_handshake_imprint_with_digest(uint8_t *buf, int encrypted)
{
    int ret, digest_pos;

    if (encrypted)
        digest_pos = ff_rtmp_calc_digest_pos(buf, 772, 728, 776);
    else
        digest_pos = ff_rtmp_calc_digest_pos(buf, 8, 728, 12);

    ret = ff_rtmp_calc_digest(buf, RTMP_HANDSHAKE_PACKET_SIZE, digest_pos,
                              rtmp_player_key, PLAYER_KEY_OPEN_PART_LEN,
                              buf + digest_pos);
    if (ret < 0)
        return ret;

    return digest_pos;
}

AMODULE_API int
rtmp_gen_c0c1(RTMPCtx *rt, unsigned char tosend[RTMP_HANDSHAKE_PACKET_SIZE+1])
{
	memcpy(tosend, rtmp_c0, sizeof(rtmp_c0));

	AVLFG rnd;
	av_lfg_init(&rnd, 0xDEADC0DE);

	// generate handshake packet - 1536 bytes of pseudorandom data
	for (int i = 9; i <= RTMP_HANDSHAKE_PACKET_SIZE; i++)
		tosend[i] = av_lfg_get(&rnd) >> 24;

	if (/*CONFIG_FFRTMPCRYPT_PROTOCOL && */rt->encrypted) {
		/* When the client wants to use RTMPE, we have to change the command
		* byte to 0x06 which means to use encrypted data and we have to set
		* the flash version to at least 9.0.115.0. */
		tosend[0] = 6;
		tosend[5] = 128;
		tosend[6] = 0;
		tosend[7] = 3;
		tosend[8] = 2;

		/* Initialize the Diffie-Hellmann context and generate the public key
		* to send to the server. */
		int ret = ff_rtmpe_gen_pub_key(rt, tosend + 1);
		if (ret < 0)
			return ret;
	}

	rt->c0c1_pos = rtmp_handshake_imprint_with_digest(tosend + 1, rt->encrypted);
	if (rt->c0c1_pos < 0)
		return rt->c0c1_pos;
	return 0;
}

#define SERVER_KEY_OPEN_PART_LEN 36   ///< length of partial key used for first server digest signing
/** Key used for RTMP server digest signing */
static const uint8_t rtmp_server_key[] = {
	'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
	'F', 'l', 'a', 's', 'h', ' ', 'M', 'e', 'd', 'i', 'a', ' ',
	'S', 'e', 'r', 'v', 'e', 'r', ' ', '0', '0', '1',

	0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
	0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
	0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
};

/**
 * Verify that the received server response has the expected digest value.
 *
 * @param buf handshake data received from the server (1536 bytes)
 * @param off position to search digest offset from
 * @return 0 if digest is valid, digest position otherwise
 */
static int rtmp_validate_digest(uint8_t *buf, int off)
{
    uint8_t digest[32];
    int ret, digest_pos;

    digest_pos = ff_rtmp_calc_digest_pos(buf, off, 728, off + 4);

    ret = ff_rtmp_calc_digest(buf, RTMP_HANDSHAKE_PACKET_SIZE, digest_pos,
                              rtmp_server_key, SERVER_KEY_OPEN_PART_LEN,
                              digest);
    if (ret < 0)
        return ret;

    if (!memcmp(digest, buf + digest_pos, 32))
        return digest_pos;
    return 0;
}

static int rtmp_calc_swf_verification(RTMPCtx *rt, uint8_t *buf)
{
	uint8_t *p;
	int ret;

	if (rt->swfhash_len != 32) {
		//av_log(s, AV_LOG_ERROR,
		//	"Hash of the decompressed SWF file is not 32 bytes long.\n");
		return AVERROR(EINVAL);
	}

	p = (uint8_t*)&rt->swfverification[0];
	bytestream_put_byte(&p, 1);
	bytestream_put_byte(&p, 1);
	bytestream_put_be32(&p, rt->swfsize);
	bytestream_put_be32(&p, rt->swfsize);

	if ((ret = ff_rtmp_calc_digest((const uint8_t*)rt->swfhash, 32, 0, buf, 32, p)) < 0)
		return ret;

	return 0;
}

AMODULE_API int
rtmp_gen_c2(RTMPCtx *rt, unsigned char tosend[1+RTMP_HANDSHAKE_PACKET_SIZE],
	    const unsigned char serverdata[RTMP_HANDSHAKE_PACKET_SIZE+1],
	    const unsigned char clientdata[RTMP_HANDSHAKE_PACKET_SIZE])
{
	int type = 0;
	uint8_t digest[32], signature[32];

	if (rt->is_input && serverdata[5] >= 3) {
		int server_pos = rtmp_validate_digest(serverdata + 1, 772);
		if (server_pos < 0)
			return server_pos;

		if (!server_pos) {
			type = 1;
			server_pos = rtmp_validate_digest(serverdata + 1, 8);
			if (server_pos < 0)
				return server_pos;

			if (!server_pos) {
				//av_log(s, AV_LOG_ERROR, "Server response validating failed\n");
				return AVERROR(EIO);
			}
		}

		/* Generate SWFVerification token (SHA256 HMAC hash of decompressed SWF,
		* key are the last 32 bytes of the server handshake. */
		if (rt->swfsize) {
			int ret = rtmp_calc_swf_verification(rt, serverdata + 1 +
				RTMP_HANDSHAKE_PACKET_SIZE - 32);
			if (ret < 0)
				return ret;
		}

		int ret = ff_rtmp_calc_digest(tosend + 1 + rt->c0c1_pos, 32, 0,
			rtmp_server_key, sizeof(rtmp_server_key),
			digest);
		if (ret < 0)
			return ret;

		ret = ff_rtmp_calc_digest(clientdata, RTMP_HANDSHAKE_PACKET_SIZE - 32,
			0, digest, 32, signature);
		if (ret < 0)
			return ret;

		if (/*CONFIG_FFRTMPCRYPT_PROTOCOL && */rt->encrypted) {
			/* Compute the shared secret key sent by the server and initialize
			* the RC4 encryption. */
			if ((ret = ff_rtmpe_compute_secret_key(rt, serverdata + 1,
				tosend + 1, type)) < 0)
				return ret;

			/* Encrypt the signature received by the server. */
			ff_rtmpe_encrypt_sig(rt, signature, digest, serverdata[0]);
		}

		if (memcmp(signature, clientdata + RTMP_HANDSHAKE_PACKET_SIZE - 32, 32)) {
			av_log(s, AV_LOG_ERROR, "Signature mismatch\n");
			return AVERROR(EIO);
		}

		for (i = 0; i < RTMP_HANDSHAKE_PACKET_SIZE; i++)
			tosend[i] = av_lfg_get(&rnd) >> 24;
		ret = ff_rtmp_calc_digest(serverdata + 1 + server_pos, 32, 0,
			rtmp_player_key, sizeof(rtmp_player_key),
			digest);
		if (ret < 0)
			return ret;

		ret = ff_rtmp_calc_digest(tosend, RTMP_HANDSHAKE_PACKET_SIZE - 32, 0,
			digest, 32,
			tosend + RTMP_HANDSHAKE_PACKET_SIZE - 32);
		if (ret < 0)
			return ret;

		if (/*CONFIG_FFRTMPCRYPT_PROTOCOL && */rt->encrypted) {
			/* Encrypt the signature to be send to the server. */
			ff_rtmpe_encrypt_sig(rt, tosend +
				RTMP_HANDSHAKE_PACKET_SIZE - 32, digest,
				serverdata[0]);
		}
#if 0
		// write reply back to the server
		if ((ret = ffurl_write(rt->stream, tosend,
			RTMP_HANDSHAKE_PACKET_SIZE)) < 0)
			return ret;
#endif
		if (/*CONFIG_FFRTMPCRYPT_PROTOCOL && */rt->encrypted) {
			/* Set RC4 keys for encryption and update the keystreams. */
			if ((ret = ff_rtmpe_update_keystream(rt)) < 0)
				return ret;
		}
	} else {
		if (/*CONFIG_FFRTMPCRYPT_PROTOCOL && */rt->encrypted) {
			/* Compute the shared secret key sent by the server and initialize
			* the RC4 encryption. */
			if ((ret = ff_rtmpe_compute_secret_key(rt, serverdata + 1,
				tosend + 1, 1)) < 0)
				return ret;

			if (serverdata[0] == 9) {
				/* Encrypt the signature received by the server. */
				ff_rtmpe_encrypt_sig(rt, signature, digest,
					serverdata[0]);
			}
		}
#if 0
		if ((ret = ffurl_write(rt->stream, serverdata + 1,
			RTMP_HANDSHAKE_PACKET_SIZE)) < 0)
			return ret;
#else
		memcmp(tosend+1, serverdata+1, RTMP_HANDSHAKE_PACKET_SIZE);
#endif
		if (/*CONFIG_FFRTMPCRYPT_PROTOCOL && */rt->encrypted) {
			/* Set RC4 keys for encryption and update the keystreams. */
			if ((ret = ff_rtmpe_update_keystream(rt)) < 0)
				return ret;
		}
	}

	return 0;
}
