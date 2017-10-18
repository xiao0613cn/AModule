// http://roxlu.com/2014/042/using-openssl-with-memory-bios

#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#ifdef _WIN32
#pragma comment(lib, "D:\\OpenSSL-Win32\\lib\\VC\\libssl32MT.lib")
#pragma comment(lib, "D:\\OpenSSL-Win32\\lib\\VC\\libcrypto32MT.lib")
#endif

struct SSLReq {
	BIO      *bio;
	AMessage  msg;
	AMessage *from;
};

enum SSLOpenState {
	S_invalid = 0,
	S_init,
	S_input,
	S_output,
	S_opened,
};

#define SSL_NON_BUFFER  1

struct SSL_IO : public AObject {
	SSL_CTX  *ssl_ctx;
	SSL      *ssl;
	BOOL      is_client;
	SSLReq    input;
	SSLReq    output;
	BUF_MEM  *outbuf;
	BUF_MEM  *outread;
	IOObject *io;
	SSLOpenState state;
#if !SSL_NON_BUFFER
	char      inbuf[3096];
	char      outdata[3096];
#endif
};

static int krx_ssl_verify_peer(int ok, X509_STORE_CTX* ctx)
{
	if (!ok) {
		int depth = X509_STORE_CTX_get_error_depth(ctx);
		int err = X509_STORE_CTX_get_error(ctx);
		TRACE("depth = %d, error = %d, string = %s.\n",
			depth, err, X509_verify_cert_error_string(err));
		ok = 1;
	}
	return 1;
}

static void krx_ssl_info_callback(const SSL* ssl, int where, int ret, const char* name) {
	TRACE("ssl info, where = %x, ret = %d, name = %s.\n", where, ret, name);
}
static void krx_ssl_server_info_callback(const SSL* ssl, int where, int ret) {
	krx_ssl_info_callback(ssl, where, ret, "server");
}
static void krx_ssl_client_info_callback(const SSL* ssl, int where, int ret) {
	krx_ssl_info_callback(ssl, where, ret, "client");
}

static inline int SSL_do_input(SSL_IO *sc)
{
#if SSL_NON_BUFFER
	// BIO_read(), update read pointer
	BUF_MEM *bm = NULL;
	BIO_get_mem_ptr(sc->input.bio, &bm);
	sc->input.msg.init(ioMsgType_Block, bm->data, bm->length);

	bm->data += bm->length;
	bm->length = 0;
	BIO_clear_retry_flags(sc->input.bio);
#else
	int result = BIO_read(sc->input.bio, sc->inbuf, sizeof(sc->inbuf));
	if (result <= 0)
		return -EIO;
	sc->input.msg.init(ioMsgType_Block, sc->inbuf, result);
#endif
	return sc->io->input(&sc->input.msg);
}

static inline int SSL_do_output(SSL_IO *sc, AMessage &msg)
{
#if SSL_NON_BUFFER
	// update write pointer
	BIO_get_mem_ptr(sc->output.bio, &sc->outread);
	assert(sc->outread->data+sc->outread->length == sc->outbuf->data+sc->outbuf->length);
	msg.init(0, sc->outbuf->data+sc->outbuf->length, sc->outbuf->max-sc->outbuf->length);
#else
	msg.init(0, sc->outdata, sizeof(sc->outdata));
#endif
	return sc->io->output(&msg);
}

static inline int SSL_on_output(SSL_IO *sc, AMessage &msg)
{
#if SSL_NON_BUFFER
	// BIO_write(), update read pointer
	sc->outbuf->length += msg.size;
	*sc->outread = *sc->outbuf;
	BIO_clear_retry_flags(sc->output.bio);
	return msg.size;
#else
	return BIO_write(sc->output.bio, msg.data, msg.size);
#endif
}

static int SSLOpenStatus(SSL_IO *sc, int result)
{
	while (result > 0) {
	switch (sc->state)
	{
	case S_init:
		if (SSL_is_init_finished(sc->ssl)
		 || ((result = SSL_do_handshake(sc->ssl)) > 0)) {
			assert(SSL_is_init_finished(sc->ssl));
			sc->state = S_opened;
			return result;
		}
		if (BIO_ctrl_pending(sc->input.bio) > 0) {
			result = SSL_do_input(sc);
			break;
		}

		result = SSL_get_error(sc->ssl, result);
		if (result == SSL_ERROR_WANT_READ) {
			sc->state = S_output;
			result = SSL_do_output(sc, sc->input.msg);
			break;
		}
		if (result != SSL_ERROR_WANT_WRITE) {
			TRACE("ssl unknown error = %d.\n", result);
			return -EIO;
		}
		result = 1;
		break;

	case S_output:
		result = SSL_on_output(sc, sc->input.msg);
		sc->state = S_init;
		break;

	default: assert(0); return -EACCES;
	}
	}
	return result;
}
static int SSLOpen(AObject *object, AMessage *msg)
{
	SSL_IO *sc = (SSL_IO*)object;
	if (msg->type != AMsgType_Option)
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	if (sc->ssl_ctx == NULL) {
		sc->is_client = option->getInt("is_client", TRUE);

		sc->ssl_ctx = SSL_CTX_new(sc->is_client ? TLS_client_method() : TLS_server_method());
		if (sc->ssl_ctx == NULL)
			return -ENOENT;

		int result = SSL_CTX_set_cipher_list(sc->ssl_ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		if (result != 1) {
			TRACE("Error: cannot set the cipher list.\n");
			//ERR_print_errors_fp(stderr);
			return -ENOENT;
		}

		SSL_CTX_set_verify(sc->ssl_ctx, SSL_VERIFY_PEER, &krx_ssl_verify_peer);
		SSL_CTX_set_verify_depth(sc->ssl_ctx, 9);
		SSL_CTX_set_mode(sc->ssl_ctx, SSL_MODE_ASYNC/*|SSL_MODE_AUTO_RETRY*/);
		SSL_CTX_set_session_cache_mode(sc->ssl_ctx, SSL_SESS_CACHE_OFF);

		/*result = SSL_CTX_set_tlsext_use_srtp(sc->ssl_ctx, "SRTP_AES128_CM_SHA1_80");
		if (result != 0) {
			TRACE("Error: cannot setup srtp.\n");
			return -3;
		}*/
	}

	sc->ssl = SSL_new(sc->ssl_ctx);
	if (sc->ssl == NULL) {
		TRACE("Error: cannot create new SSL*.\n");
		return -ENOENT;
	}

	sc->input.bio = BIO_new(BIO_s_mem());
	sc->output.bio = BIO_new(BIO_s_mem());
	if (sc->outbuf == NULL) {
		sc->outbuf = BUF_MEM_new();
		BUF_MEM_grow(sc->outbuf, 2048);
	}
	BUF_MEM_grow(sc->outbuf, 0);
	BIO_set_mem_buf(sc->output.bio, sc->outbuf, FALSE);

	SSL_set_bio(sc->ssl, sc->output.bio, sc->input.bio);

	/* either use the server or client part of the protocol */
	if (sc->is_client) {
		SSL_set_info_callback(sc->ssl, &krx_ssl_client_info_callback);
		SSL_set_connect_state(sc->ssl);
	} else {
		SSL_set_info_callback(sc->ssl, &krx_ssl_server_info_callback);
		SSL_set_accept_state(sc->ssl);
	}

	AOption *io_opt = option->find("io");
	if (sc->io == NULL) {
		int result = sc->create(&sc->io, sc, io_opt, "async_tcp");
		if (result < 0)
			return result;
	}

	sc->input.msg.init(io_opt);
	sc->input.msg.done = &TObjectDone(SSL_IO, input.msg, input.from, SSLOpenStatus);
	sc->input.from = msg;

	sc->state = S_init;
	int result = sc->io->open(&sc->input.msg);
	if (result != 0)
		result = SSLOpenStatus(sc, result);
	return result;
}

static int SSLSetOpt(AObject *object, AOption *option)
{
	SSL_IO *sc = (SSL_IO*)object;
	if (sc->io == NULL)
		return -ENOENT;
	return sc->io->setopt(option);
}

static int SSLGetOpt(AObject *object, AOption *option)
{
	SSL_IO *sc = (SSL_IO*)object;
	if (sc->io == NULL)
		return -ENOENT;
	return sc->io->getopt(option);
}

static int SSLInputDone(AMessage *msg, int result)
{
	SSL_IO *sc = container_of(msg, SSL_IO, input.msg);
	sc->input.from->done2(result);
	return result;
}

static int SSLOutputStatus(SSL_IO *sc, int result)
{
	while (result > 0) {
		if (sc->output.msg.size != 0)
			result = SSL_on_output(sc, sc->output.msg);

		result = SSL_read(sc->ssl, sc->output.from->data, sc->output.from->size);
		if (result > 0) {
			sc->output.from->size = result;
			return result;
		}

		result = SSL_get_error(sc->ssl, result);
		if (result == SSL_ERROR_WANT_WRITE) {
			int len = BIO_ctrl_pending(sc->input.bio);
			assert(len != 0);
		}
		if ((result != SSL_ERROR_WANT_READ) && (result != SSL_ERROR_ZERO_RETURN)) {
			TRACE("ssl unknown error = %d.\n", result);
			return -EIO;
		}
		result = SSL_do_output(sc, sc->output.msg);
	}
	return result;
}

static int SSLRequest(AObject *object, int reqix, AMessage *msg)
{
	SSL_IO *sc = (SSL_IO*)object;
	int result;
	switch (reqix)
	{
	case Aio_Input:
		result = SSL_write(sc->ssl, msg->data, msg->size);
		if (result < 0)
			return -EIO;

		if (BIO_ctrl_pending(sc->input.bio) > 0) {
			sc->input.msg.done = &SSLInputDone;
			sc->input.from = msg;
			return SSL_do_input(sc);
		}
		return msg->size;

	case Aio_Output:
		sc->output.msg.init();
		sc->output.msg.done = &TObjectDone(SSL_IO, output.msg, output.from, SSLOutputStatus);
		sc->output.from = msg;
		return SSLOutputStatus(sc, 1);

	default: assert(0); return -EACCES;
	}
}

static int SSLCancel(AObject *object, int reqix, AMessage *msg)
{
	SSL_IO *sc = (SSL_IO*)object;
	if (sc->io == NULL)
		return -ENOENT;
	return sc->io->cancel(reqix, msg);
}

static int SSLClose(AObject *object, AMessage *msg)
{
	SSL_IO *sc = (SSL_IO*)object;
	if (sc->io == NULL)
		return -ENOENT;

	if (msg != NULL)
		if_not(sc->ssl, NULL, SSL_free);
	return sc->io->close(msg);
}

static int SSLCreate(AObject **object, AObject *parent, AOption *option)
{
	SSL_IO *sc = (SSL_IO*)*object;
	sc->ssl_ctx = NULL; sc->ssl = NULL;
	sc->io = NULL; sc->outbuf = NULL;
	return 1;
}

static void SSLRelease(AObject *object)
{
	SSL_IO *sc = (SSL_IO*)object;
	if_not(sc->ssl_ctx, NULL, SSL_CTX_free);
	if_not(sc->ssl, NULL, SSL_free);
	if_not(sc->outbuf, NULL, BUF_MEM_free);
	release_s(sc->io);
}

static int SSLInit(AOption *global_option, AOption *module_option, BOOL first)
{
	if (first) {
		SSL_library_init();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
		//ERR_load_BIO_strings();
	}
	return 0;
}

static void SSLExit(BOOL inited)
{
	if (inited) {
		//ERR_remove_state(0);
		//ENGINE_cleanup();
		//CONF_modules_unload(1);
		//ERR_free_strings();
		EVP_cleanup();
		//sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
		CRYPTO_cleanup_all_ex_data();
	}
}

IOModule OpenSSLModule = { {
	"io",
	"io_openssl",
	sizeof(SSL_IO),
	&SSLInit, NULL,
	&SSLCreate, &SSLRelease, NULL, },
	&SSLOpen,
	&SSLSetOpt, &SSLGetOpt,
	&SSLRequest,
	&SSLCancel,
	&SSLClose,
};

static auto_reg_t reg(OpenSSLModule.module);
