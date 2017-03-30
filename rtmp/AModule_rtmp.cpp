#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_rtmp.h"
extern "C" {
#include "lfg.h"
#include "sha.h"
#include "md5.h"
#include "bytestream.h"
#include "rtmppkt.h"
#if CONFIG_FFRTMPCRYPT_PROTOCOL
#include "rtmpdh.h"
#include "rtmpcrypt.h"
#endif
};

/**
 * emulated Flash client version - 9.0.124.2 on Linux
 * @{
 */
#define RTMP_CLIENT_PLATFORM "LNX"
#define RTMP_CLIENT_VER1    9
#define RTMP_CLIENT_VER2    0
#define RTMP_CLIENT_VER3  124
#define RTMP_CLIENT_VER4    2

AMODULE_API void
rtmp_init(RTMPCtx *rt, int is_input)
{
	memset(rt, 0, sizeof(*rt));
	rt->out_chunk_size = 128;
	rt->in_chunk_size  = 128; // Probably overwritten later

	rt->is_input = is_input;
	if (is_input) {
		snprintf(rt->flashver, sizeof(rt->flashver),
			"%s %d,%d,%d,%d",
			RTMP_CLIENT_PLATFORM, RTMP_CLIENT_VER1, RTMP_CLIENT_VER2,
			RTMP_CLIENT_VER3, RTMP_CLIENT_VER4);
	} else {
		snprintf(rt->flashver, sizeof(rt->flashver),
			"FMLE/3.0 (compatible; %s %d,%d,%d,%d)",
			RTMP_CLIENT_PLATFORM, RTMP_CLIENT_VER1, RTMP_CLIENT_VER2,
			RTMP_CLIENT_VER3, RTMP_CLIENT_VER4);
	}
}

const unsigned char rtmp_c0[] = {
	3,                // unencrypted data
	0, 0, 0, 0,       // client uptime
	RTMP_CLIENT_VER1,
	RTMP_CLIENT_VER2,
	RTMP_CLIENT_VER3,
	RTMP_CLIENT_VER4,
};

int ff_rtmp_calc_digest_pos(const uint8_t *buf, int off, int mod_val, int add_val)
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

	//sha = av_sha_alloc();
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
#if CONFIG_FFRTMPCRYPT_PROTOCOL
	if (CONFIG_FFRTMPCRYPT_PROTOCOL && rt->encrypted) {
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
#endif
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
		av_log(s, AV_LOG_ERROR,
			"Hash of the decompressed SWF file is not 32 bytes long.\n");
		return AVERROR(EINVAL);
	}

	p = (uint8_t*)&rt->swfverification[0];
	bytestream_put_byte(&p, 1);
	bytestream_put_byte(&p, 1);
	bytestream_put_be32(&p, rt->swfsize);
	bytestream_put_be32(&p, rt->swfsize);

	if ((ret = ff_rtmp_calc_digest(rt->swfhash, 32, 0, buf, 32, p)) < 0)
		return ret;

	return 0;
}

AMODULE_API int
rtmp_gen_c2(RTMPCtx *rt, unsigned char tosend[1+RTMP_HANDSHAKE_PACKET_SIZE],
            const unsigned char serverdata[RTMP_HANDSHAKE_PACKET_SIZE+1],
            const unsigned char clientdata[RTMP_HANDSHAKE_PACKET_SIZE])
{
	int ret;
	int type = 0;
	uint8_t digest[32], signature[32];

	if (rt->is_input && serverdata[5] >= 3) {
		int server_pos = rtmp_validate_digest((uint8_t*)serverdata + 1, 772);
		if (server_pos < 0)
			return server_pos;

		if (!server_pos) {
			type = 1;
			server_pos = rtmp_validate_digest((uint8_t*)serverdata + 1, 8);
			if (server_pos < 0)
				return server_pos;

			if (!server_pos) {
				av_log(s, AV_LOG_ERROR, "Server response validating failed\n");
				return AVERROR(EIO);
			}
		}

		/* Generate SWFVerification token (SHA256 HMAC hash of decompressed SWF,
		* key are the last 32 bytes of the server handshake. */
		if (rt->swfsize) {
			if ((ret = rtmp_calc_swf_verification(rt, (uint8_t*)serverdata + 1 +
			                                      RTMP_HANDSHAKE_PACKET_SIZE - 32)) < 0)
				return ret;
		}

		ret = ff_rtmp_calc_digest(tosend + 1 + rt->c0c1_pos, 32, 0,
			rtmp_server_key, sizeof(rtmp_server_key),
			digest);
		if (ret < 0)
			return ret;

		ret = ff_rtmp_calc_digest(clientdata, RTMP_HANDSHAKE_PACKET_SIZE - 32,
			0, digest, 32, signature);
		if (ret < 0)
			return ret;
#if CONFIG_FFRTMPCRYPT_PROTOCOL
		if (CONFIG_FFRTMPCRYPT_PROTOCOL && rt->encrypted) {
			/* Compute the shared secret key sent by the server and initialize
			* the RC4 encryption. */
			if ((ret = ff_rtmpe_compute_secret_key(rt, serverdata + 1,
				tosend + 1, type)) < 0)
				return ret;

			/* Encrypt the signature received by the server. */
			ff_rtmpe_encrypt_sig(rt, signature, digest, serverdata[0]);
		}
#endif
		if (memcmp(signature, clientdata + RTMP_HANDSHAKE_PACKET_SIZE - 32, 32)) {
			av_log(s, AV_LOG_ERROR, "Signature mismatch\n");
			return AVERROR(EIO);
		}

		//////////////////////////////////////////////////////////////////////////
		tosend += 1;
		AVLFG rnd;
		av_lfg_init(&rnd, 0xDEADC0DE);
		for (int i = 0; i < RTMP_HANDSHAKE_PACKET_SIZE; i++)
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
#if CONFIG_FFRTMPCRYPT_PROTOCOL
		if (CONFIG_FFRTMPCRYPT_PROTOCOL && rt->encrypted) {
			/* Encrypt the signature to be send to the server. */
			ff_rtmpe_encrypt_sig(rt, tosend +
				RTMP_HANDSHAKE_PACKET_SIZE - 32, digest,
				serverdata[0]);
		}

		// write reply back to the server
		/*if ((ret = ffurl_write(rt->stream, tosend,
			RTMP_HANDSHAKE_PACKET_SIZE)) < 0)
			return ret;*/

		if (CONFIG_FFRTMPCRYPT_PROTOCOL && rt->encrypted) {
			/* Set RC4 keys for encryption and update the keystreams. */
			if ((ret = ff_rtmpe_update_keystream(rt)) < 0)
				return ret;
		}
#endif
	} else {
#if CONFIG_FFRTMPCRYPT_PROTOCOL
		if (CONFIG_FFRTMPCRYPT_PROTOCOL && rt->encrypted) {
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

		/*if ((ret = ffurl_write(rt->stream, serverdata + 1,
			RTMP_HANDSHAKE_PACKET_SIZE)) < 0)
			return ret;*/

		if (CONFIG_FFRTMPCRYPT_PROTOCOL && rt->encrypted) {
			/* Set RC4 keys for encryption and update the keystreams. */
			if ((ret = ff_rtmpe_update_keystream(rt)) < 0)
				return ret;
		}
#endif
		memcmp(tosend+1, serverdata+1, RTMP_HANDSHAKE_PACKET_SIZE);
	}

	return 0;
}

/**
 * rtmp handshake server side
 */
AMODULE_API int
rtmp_gen_s0s1s2(RTMPCtx *rt, unsigned char s0s1[1+RTMP_HANDSHAKE_PACKET_SIZE],
                unsigned char s2[RTMP_HANDSHAKE_PACKET_SIZE],
                const unsigned char c0c1[1+RTMP_HANDSHAKE_PACKET_SIZE])
{
    uint32_t hs_epoch;
    uint32_t hs_my_epoch;
    const uint8_t *hs_c1 = c0c1 + 1;
    uint8_t *hs_s1 = s0s1 + 1;
    uint32_t zeroes;
    AVLFG rnd;
    int randomidx;
//    uint32_t temp       = 0;
//    int inoutsize       = 0;
//    int ret;

    /*inoutsize = ffurl_read_complete(rt->stream, buffer, 1);       // Receive C0
    if (inoutsize <= 0) {
        av_log(s, AV_LOG_ERROR, "Unable to read handshake\n");
        return AVERROR(EIO);
    }*/
    // Check Version
    if (c0c1[0] != 3) {
        av_log(s, AV_LOG_ERROR, "RTMP protocol version mismatch\n");
        return AVERROR(EIO);
    }
    s0s1[0] = c0c1[0];
    /*if (ffurl_write(rt->stream, buffer, 1) <= 0) {                 // Send S0
        av_log(s, AV_LOG_ERROR,
               "Unable to write answer - RTMP S0\n");
        return AVERROR(EIO);
    }*/

    /* Receive C1 */
    /*ret = rtmp_receive_hs_packet(rt, &hs_epoch, &zeroes, hs_c1,
                                 RTMP_HANDSHAKE_PACKET_SIZE);
    if (ret) {
        av_log(s, AV_LOG_ERROR, "RTMP Handshake C1 Error\n");
        return ret;
    }*/
    hs_epoch = AV_RB32(hs_c1);
    zeroes = AV_RB32(hs_c1+4);

    /* Send S1 */
    /* By now same epoch will be sent */
    hs_my_epoch = hs_epoch;

    av_lfg_init(&rnd, 0xDEADC0DE);
    for (randomidx = 8; randomidx < (RTMP_HANDSHAKE_PACKET_SIZE); randomidx+=4)
	    AV_WB32(hs_s1+randomidx, av_lfg_get(&rnd));

    AV_WB32(hs_s1, hs_my_epoch);
    AV_WB32(hs_s1+4, 0);
    /* Generate random */
    /*for (randomidx = 8; randomidx < (RTMP_HANDSHAKE_PACKET_SIZE);
         randomidx += 4)
        AV_WB32(hs_s1 + randomidx, av_get_random_seed());

    ret = rtmp_send_hs_packet(rt, hs_my_epoch, 0, hs_s1,
                              RTMP_HANDSHAKE_PACKET_SIZE);
    if (ret) {
        av_log(s, AV_LOG_ERROR, "RTMP Handshake S1 Error\n");
        return ret;
    }*/

    /* Send S2 */
    /*ret = rtmp_send_hs_packet(rt, hs_epoch, 0, hs_c1,
                              RTMP_HANDSHAKE_PACKET_SIZE);
    if (ret) {
        av_log(s, AV_LOG_ERROR, "RTMP Handshake S2 Error\n");
        return ret;
    }*/
    memcpy(s2+8, hs_c1+8, RTMP_HANDSHAKE_PACKET_SIZE-8);
    AV_WB32(s2, hs_epoch);
    AV_WB32(s2+4, 0);
    return 0;
}

AMODULE_API int
rtmp_check_c2(RTMPCtx *rt, const unsigned char buffer[RTMP_HANDSHAKE_PACKET_SIZE],
	      const unsigned char hs_s1[RTMP_HANDSHAKE_PACKET_SIZE])
{
    /* Receive C2 */
    /*ret = rtmp_receive_hs_packet(rt, &temp, &zeroes, buffer,
                                 RTMP_HANDSHAKE_PACKET_SIZE);
    if (ret) {
        av_log(s, AV_LOG_ERROR, "RTMP Handshake C2 Error\n");
        return ret;
    }*/
    uint32_t temp = AV_RB32(buffer);
    uint32_t zeroes = AV_RB32(buffer+4);

    uint32_t hs_my_epoch = AV_RB32(hs_s1);

    if (temp != hs_my_epoch) {
        av_log(s, AV_LOG_WARNING,
               "Erroneous C2 Message epoch does not match up with C1 epoch\n");
	return -1;
    }
    if (memcmp(buffer + 8, hs_s1 + 8,
	    RTMP_HANDSHAKE_PACKET_SIZE - 8)) {
        av_log(s, AV_LOG_WARNING,
               "Erroneous C2 Message random does not match up\n");
	return -2;
    }
    return 0;
}

AMODULE_API int
rtmp_calc_swfhash(RTMPCtx *rt, const uint8_t *swfdata, int swfsize)
{
	if (swfsize <= 0) {
		rt->swfsize = 0;
		rt->swfhash_len = 0;
		return 0;
	}
	int ret;
	ret = ff_rtmp_calc_digest(swfdata, swfsize, 0,
		(const uint8_t*)"Genuine Adobe Flash Player 001", 30, rt->swfhash);
	if (ret < 0)
		return ret;

	rt->swfsize = swfsize;
	rt->swfhash_len = 32;
	return 0;
}

static int rtmp_write_amf_data(char *param, uint8_t **p)
{
    char *field, *value;
    char type;

    /* The type must be B for Boolean, N for number, S for string, O for
     * object, or Z for null. For Booleans the data must be either 0 or 1 for
     * FALSE or TRUE, respectively. Likewise for Objects the data must be
     * 0 or 1 to end or begin an object, respectively. Data items in subobjects
     * may be named, by prefixing the type with 'N' and specifying the name
     * before the value (ie. NB:myFlag:1). This option may be used multiple times
     * to construct arbitrary AMF sequences. */
    if (param[0] && param[1] == ':') {
        type = param[0];
        value = param + 2;
    } else if (param[0] == 'N' && param[1] && param[2] == ':') {
        type = param[1];
        field = param + 3;
        value = strchr(field, ':');
        if (!value)
            goto fail;
        *value = '\0';
        value++;

        ff_amf_write_field_name(p, field, value-field-1);
    } else {
        goto fail;
    }

    switch (type) {
    case 'B':
        ff_amf_write_bool(p, value[0] != '0');
        break;
    case 'S':
        ff_amf_write_string(p, value, strlen(value));
        break;
    case 'N':
        ff_amf_write_number(p, strtod(value, NULL));
        break;
    case 'Z':
        ff_amf_write_null(p);
        break;
    case 'O':
        if (value[0] != '0')
            ff_amf_write_object_start(p);
        else
            ff_amf_write_object_end(p);
        break;
    default:
        goto fail;
        break;
    }

    return 0;

fail:
    av_log(s, AV_LOG_ERROR, "Invalid AMF parameter: %s\n", param);
    return AVERROR(EINVAL);
}

/**
 * Generate 'connect' call and send it to the server.
 */
AMODULE_API int
rtmp_gen_connect(RTMPCtx *rt, uint8_t *data, const char **app, const char **tcurl, char *param)
{
    uint8_t *p;
    int ret;

    /*if ((ret = ff_rtmp_packet_create(&pkt, RTMP_SYSTEM_CHANNEL, RTMP_PT_INVOKE,
                                     0, 4096 + APP_MAX_LENGTH)) < 0)
        return ret;*/
    p = data;

    ff_amf_write_string_sz(&p, "connect");
    ff_amf_write_number(&p, ++rt->nb_invokes);
    ff_amf_write_object_start(&p);
    if (app) {
        ff_amf_write_field_name_sz(&p, "app");
        ff_amf_write_string2(&p, app[0], app[1]); //rt->app, rt->auth_params);
    }

    if (!rt->is_input) {
        ff_amf_write_field_name_sz(&p, "type");
        ff_amf_write_string_sz(&p, "nonprivate");
    }
    ff_amf_write_field_name_sz(&p, "flashVer");
    ff_amf_write_string(&p, rt->flashver, strlen(rt->flashver));

    if (rt->swfurl) {
        ff_amf_write_field_name_sz(&p, "swfUrl");
        ff_amf_write_string(&p, rt->swfurl, strlen(rt->swfurl));
    }
    if (tcurl) {
        ff_amf_write_field_name_sz(&p, "tcUrl");
        ff_amf_write_string2(&p, tcurl[0], tcurl[1]); //rt->tcurl, rt->auth_params);
    }
    if (rt->is_input) {
        ff_amf_write_field_name_sz(&p, "fpad");
        ff_amf_write_bool(&p, 0);
        ff_amf_write_field_name_sz(&p, "capabilities");
        ff_amf_write_number(&p, 15.0);

        /* Tell the server we support all the audio codecs except
         * SUPPORT_SND_INTEL (0x0008) and SUPPORT_SND_UNUSED (0x0010)
         * which are unused in the RTMP protocol implementation. */
        ff_amf_write_field_name_sz(&p, "audioCodecs");
        ff_amf_write_number(&p, 4071.0);
        ff_amf_write_field_name_sz(&p, "videoCodecs");
        ff_amf_write_number(&p, 252.0);
        ff_amf_write_field_name_sz(&p, "videoFunction");
        ff_amf_write_number(&p, 1.0);

        if (rt->pageurl) {
            ff_amf_write_field_name_sz(&p, "pageUrl");
            ff_amf_write_string(&p, rt->pageurl, strlen(rt->pageurl));
        }
    }
    ff_amf_write_object_end(&p);

    if (param) {
        // Write arbitrary AMF data to the Connect message.
        while (1) {
            char *sep;
            param += strspn(param, " ");
            if (!*param)
                break;
            sep = strchr(param, ' ');
            if (sep)
                *sep = '\0';
            if ((ret = rtmp_write_amf_data(param, &p)) < 0) {
                // Invalid AMF parameter.
                //ff_rtmp_packet_destroy(&pkt);
                return ret;
            }

            if (sep)
                param = sep + 1;
            else
                break;
        }
    }
    return p - data;
    //pkt->size = p - pkt->data;
    //return rtmp_send_packet(rt, &pkt, 1);
}

AMODULE_API int
rtmp_parse_one_chunk(RTMPCtx *rt, RTMPPacket *p, RTMPPacket *prev_pkt)
{
	int channel_id, timestamp, size;
	uint32_t ts_field; // non-extended timestamp or delta field
	uint32_t extra = 0;
	enum RTMPPacketType type;
	int written = 0;
	int ret, toread;

	uint8_t hdr = p->data[0];
	assert(p->size > 1);
	written++;
	channel_id = hdr & 0x3F;

	if (channel_id < 2) { //special case for channel number >= 64
		if (p->size < written+channel_id+1)
			return 0;
		channel_id = channel_id?AV_RL16(p->data+written):AV_RL8(p->data+written) + 64;
		written += channel_id + 1;
	}
	if ((channel_id != p->channel_id)
	 || (prev_pkt == NULL)
	 || (channel_id != prev_pkt->channel_id)) {
		p->channel_id = channel_id;
		return 0;
	}

	size  = prev_pkt->size;
	type  = prev_pkt->type;
	extra = prev_pkt->extra;

	hdr >>= 6; // header size indicator
	if (hdr == RTMP_PS_ONEBYTE) {
		ts_field = prev_pkt->ts_field;
	} else {
		if (p->size < written+3)
			return 0;
		ts_field = AV_RB24(p->data+written);
		written += 3;
		if (hdr != RTMP_PS_FOURBYTES) {
			if (p->size < written+3)
				return 0;
			size = AV_RB24(p->data+written);
			written += 3;
			if (p->size < written+1)
				return 0;
			type = (RTMPPacketType)p->data[written];
			written++;
			if (hdr == RTMP_PS_TWELVEBYTES) {
				if (p->size < written+4)
					return 0;
				extra = AV_RL32(p->data+written);
				written += 4;
			}
		}
	}
	if (ts_field == 0xFFFFFF) {
		if (p->size < written+4)
			return 0;
		timestamp = AV_RB32(p->data+written);
		written += 4;
	} else {
		timestamp = ts_field;
	}
	if (hdr != RTMP_PS_TWELVEBYTES)
		timestamp += prev_pkt->timestamp;

	if (prev_pkt->read && size != prev_pkt->size) {
		av_log(NULL, AV_LOG_ERROR, "RTMP packet size mismatch %d != %d\n",
			size,
			prev_pkt->size);
		//ff_rtmp_packet_destroy(&prev_pkt[channel_id]);
		prev_pkt->read = 0;
	}

	if (!prev_pkt->read) {
		p->type = type;
		p->timestamp = timestamp;
		p->offset = 0;
		prev_pkt->ts_field   = ts_field;
		prev_pkt->timestamp  = timestamp;
	} else {
		// previous packet in this channel hasn't completed reading
		//RTMPPacket *prev = &prev_pkt[channel_id];
		//p->data          = prev->data;
		//p->size          = prev->size;
		//p->channel_id    = prev->channel_id;
		p->type          = prev_pkt->type;
		p->ts_field      = prev_pkt->ts_field;
		p->offset        = prev_pkt->offset;
		p->timestamp     = prev_pkt->timestamp;
		//prev->data       = NULL;
	}
	p->read = prev_pkt->read + written;
	p->extra = extra;
	// save history
	prev_pkt->channel_id = channel_id;
	prev_pkt->type       = type;
	prev_pkt->data       = p->data + written;
	prev_pkt->size       = size;
	prev_pkt->extra      = extra;
	size = size - p->offset;

	toread = FFMIN(size, rt->in_chunk_size);
	if (p->size < written+toread)
		return 0;
	written   += toread;
	size      -= toread;
	p->read   += toread;
	p->offset += toread;

	if (size > 0) {
		//RTMPPacket *prev = &prev_pkt[channel_id];
		//prev->data = p->data;
		prev_pkt->read = p->read;
		prev_pkt->offset = p->offset;
		//p->data      = NULL;
		//return AVERROR(EAGAIN);
	} else {
		prev_pkt->read = 0; // read complete; reset if needed
		prev_pkt->offset = 0;
	}
	return written;
}

AMODULE_API int
rtmp_gen_chunk_head(RTMPCtx *rt, RTMPPacket *pkt, RTMPPacket *prev_pkt)
{
	uint8_t *p = pkt->data;
	int mode = RTMP_PS_TWELVEBYTES;
	int off = 0;
	int written = 0;
	int ret;
	int use_delta; // flag if using timestamp delta, not RTMP_PS_TWELVEBYTES
	uint32_t timestamp; // full 32-bit timestamp or delta value

	//if channel_id = 0, this is first presentation of prev_pkt, send full hdr.
	use_delta = prev_pkt->channel_id &&
		pkt->extra == prev_pkt->extra &&
		pkt->timestamp >= prev_pkt->timestamp;

	timestamp = pkt->timestamp;
	if (use_delta) {
		timestamp -= prev_pkt->timestamp;
	}
	if (timestamp >= 0xFFFFFF) {
		pkt->ts_field = 0xFFFFFF;
	} else {
		pkt->ts_field = timestamp;
	}

	if (use_delta) {
		if (pkt->type == prev_pkt->type
		 && pkt->size == prev_pkt->size) {
			mode = RTMP_PS_FOURBYTES;
			if (pkt->ts_field == prev_pkt->ts_field)
				mode = RTMP_PS_ONEBYTE;
		} else {
			mode = RTMP_PS_EIGHTBYTES;
		}
	}

	if (pkt->channel_id < 64) {
		bytestream_put_byte(&p, pkt->channel_id | (mode << 6));
	} else if (pkt->channel_id < 64 + 256) {
		bytestream_put_byte(&p, 0               | (mode << 6));
		bytestream_put_byte(&p, pkt->channel_id - 64);
	} else {
		bytestream_put_byte(&p, 1               | (mode << 6));
		bytestream_put_le16(&p, pkt->channel_id - 64);
	}
	if (mode != RTMP_PS_ONEBYTE) {
		bytestream_put_be24(&p, pkt->ts_field);
		if (mode != RTMP_PS_FOURBYTES) {
			bytestream_put_be24(&p, pkt->size);
			bytestream_put_byte(&p, pkt->type);
			if (mode == RTMP_PS_TWELVEBYTES)
				bytestream_put_le32(&p, pkt->extra);
		}
	}
	if (pkt->ts_field == 0xFFFFFF)
		bytestream_put_be32(&p, timestamp);
	// save history
	prev_pkt->channel_id = pkt->channel_id;
	prev_pkt->type       = pkt->type;
	prev_pkt->size       = pkt->size;
	prev_pkt->timestamp  = pkt->timestamp;
	prev_pkt->ts_field   = pkt->ts_field;
	prev_pkt->extra      = pkt->extra;
	prev_pkt->offset     = pkt->offset = 0;

	return (p - pkt->data);
	/*if ((ret = ffurl_write(h, pkt_hdr, p - pkt_hdr)) < 0)
		return ret;
	written = p - pkt_hdr + pkt->size;
	while (off < pkt->size) {
		int towrite = FFMIN(rt->out_chunk_size, pkt->size - off);
		if ((ret = ffurl_write(h, pkt->data + off, towrite)) < 0)
			return ret;
		off += towrite;
		if (off < pkt->size) {
			uint8_t marker = 0xC0 | pkt->channel_id;
			if ((ret = ffurl_write(h, &marker, 1)) < 0)
				return ret;
			written++;
			if (pkt->ts_field == 0xFFFFFF) {
				uint8_t ts_header[4];
				AV_WB32(ts_header, timestamp);
				if ((ret = ffurl_write(h, ts_header, 4)) < 0)
					return ret;
				written += 4;
			}
		}
	}
	return written;*/
}

AMODULE_API int
rtmp_gen_next_head(RTMPCtx *rt, RTMPPacket *pkt, unsigned char *data)
{
	int towrite = FFMIN(rt->out_chunk_size, pkt->size - pkt->offset);
	pkt->offset += towrite;
	if (pkt->offset >= pkt->size)
		return 0;

	int written = 1;
	data[0] = 0xC0 | pkt->channel_id;

	if (pkt->ts_field == 0xFFFFFF) {
		AV_WB32(data+written, pkt->timestamp);
		written += 4;
	}
	return written;
}

AMODULE_API int
rtmp_gen_releaseStream(RTMPCtx *rt, unsigned char *data)
{
	uint8_t *p = data;
	ff_amf_write_string_sz(&p, "releaseStream");
	ff_amf_write_number(&p, ++rt->nb_invokes);
	ff_amf_write_null(&p);
	ff_amf_write_string(&p, rt->playpath, strlen(rt->playpath));
	return p - data;
}

AMODULE_API int
rtmp_gen_FCPublish(RTMPCtx *rt, unsigned char *data)
{
	uint8_t *p = data;
	ff_amf_write_string_sz(&p, "FCPublish");
	ff_amf_write_number(&p, ++rt->nb_invokes);
	ff_amf_write_null(&p);
	ff_amf_write_string(&p, rt->playpath, strlen(rt->playpath));
	return p - data;
}

AMODULE_API int
rtmp_gen_createStream(RTMPCtx *rt, unsigned char *data)
{
	uint8_t *p = data;
	ff_amf_write_string_sz(&p, "createStream");
	ff_amf_write_number(&p, ++rt->nb_invokes);
	ff_amf_write_null(&p);
	return p - data;
}

AMODULE_API int
rtmp_gen_publish(RTMPCtx *rt, unsigned char *data)
{
	uint8_t *p = data;
	ff_amf_write_string_sz(&p, "publish");
	ff_amf_write_number(&p, ++rt->nb_invokes);
	ff_amf_write_null(&p);
	ff_amf_write_string(&p, rt->playpath, strlen(rt->playpath));
	ff_amf_write_string_sz(&p, "live");
	return p - data;
}

AMODULE_API int
rtmp_gen_deleteStream(RTMPCtx *rt, unsigned char *data)
{
	uint8_t *p = data;
	ff_amf_write_string_sz(&p, "deleteStream");
	ff_amf_write_number(&p, ++rt->nb_invokes);
	ff_amf_write_null(&p);
	ff_amf_write_number(&p, rt->stream_id);
	return p - data;
}
