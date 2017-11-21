// http://roxlu.com/2014/042/using-openssl-with-memory-bios

#include "../stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#ifdef _WIN32
#pragma comment(lib, "..\\bin\\AModule.lib")
#pragma comment(lib, "..\\win32\\openssl\\libssl32MT.lib")
#pragma comment(lib, "..\\win32\\openssl\\libcrypto32MT.lib")
#endif

struct SSLReq {
	BIO      *bio;
	AMessage  msg;
	AMessage *from;
};

enum SSLOpenState {
	S_invalid = 0,
	S_init,
	S_output,
	S_opened,
};

#define SSL_NON_BUFFER  1

struct SSL_IO : public AObject {
	SSL_CTX  *ssl_ctx;
	SSL      *ssl;
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

template <BOOL enable_self_signed_certi>
static int verify_peer(int ok, X509_STORE_CTX* ctx)
{
	if (!ok) {
		int depth = X509_STORE_CTX_get_error_depth(ctx);
		int err = X509_STORE_CTX_get_error(ctx);
		TRACE("depth = %d, error = %d, string = %s.\n",
			depth, err, X509_verify_cert_error_string(err));
		if (enable_self_signed_certi)
			ok = (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN);
	}
	return ok;
}

template <BOOL is_client>
static void info_callback(const SSL* ssl, int where, int ret)
{
	TRACE("ssl info, where = %x, ret = %d, name = %s.\n", where, ret,
		is_client ? "client" : "server");
}

static int SSL_CTX_create(SSL_CTX *&ctx, AOption *opt, BOOL is_client)
{
	assert(ctx == NULL);
	ctx = SSL_CTX_new(is_client ? TLS_client_method() : TLS_server_method());
	if (ctx == NULL)
		return -ENOMEM;

	int result = SSL_CTX_set_cipher_list(ctx, opt->getStr("cipher_list", "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"));
	if (result != 1) {
		TRACE("SSL_CTX_set_cipher_list() = %d.\n", result);
		return -ENOENT;
	}

	if (opt->getInt("enable_self_signed_certi", TRUE))
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, &verify_peer<TRUE>);
	else
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, &verify_peer<FALSE>);

	SSL_CTX_set_verify_depth(ctx, 9);
	SSL_CTX_set_mode(ctx, SSL_MODE_ASYNC/*|SSL_MODE_AUTO_RETRY*/);
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

	const char *ca_file = opt->getStr("ca_file", NULL);
	const char *ca_path = opt->getStr("ca_path", NULL);
	if ((ca_file != NULL) || (ca_path != NULL)) {
		result = SSL_CTX_load_verify_locations(ctx, ca_file, ca_path);
		if (result != 1) {
			TRACE("SSL_CTX_load_verify_locations(%s, %s) = %d.\n", ca_file, ca_path, result);
			return -ENOENT;
		}
	}

	const char *certi_file = opt->getStr("certi_file", NULL);
	if (certi_file != NULL) {
		int certi_type = opt->getInt("certi_type", SSL_FILETYPE_PEM);
		result = SSL_CTX_use_certificate_file(ctx, certi_file, certi_type);
		if (result != 1) {
			TRACE("SSL_use_certificate_file(%s, %d) = %d.\n", certi_file, certi_type, result);
			return -ENOENT;
		}
	}

	const char *key_file = opt->getStr("key_file", NULL);
	if (key_file != NULL) {
		int key_type = opt->getInt("key_type", SSL_FILETYPE_PEM);
		result = SSL_CTX_use_PrivateKey_file(ctx, key_file, key_type);
		if (result != 1) {
			TRACE("SSL_CTX_use_PrivateKey_file(%s, %d) = %d.\n", key_file, key_type, result);
			return -ENOENT;
		}

		result = SSL_CTX_check_private_key(ctx);
		if (result != 1) {
			TRACE("SSL_CTX_check_private_key() = %d.\n", result);
			return -EACCES;
		}
	}

	/*result = SSL_CTX_set_tlsext_use_srtp(sc->ssl_ctx, "SRTP_AES128_CM_SHA1_80");
	if (result != 0) {
		TRACE("Error: cannot setup srtp.\n");
		return -3;
	}*/
	return 1;
}

static int SSL_io_init(SSL_IO *sc, SSL_CTX *ctx, AOption *opt, BOOL is_client)
{
	assert(sc->ssl == NULL);
	sc->ssl = SSL_new(ctx);
	if (sc->ssl == NULL) {
		TRACE("Error: cannot create new SSL*.\n");
		return -ENOMEM;
	}

	sc->input.bio = BIO_new(BIO_s_mem());
	sc->output.bio = BIO_new(BIO_s_mem());
	if (sc->outbuf == NULL) {
		sc->outbuf = BUF_MEM_new();
		BUF_MEM_grow(sc->outbuf, opt->getInt("outbuf_size", 2048));
	}
	BUF_MEM_grow(sc->outbuf, 0);
	BIO_set_mem_buf(sc->output.bio, sc->outbuf, FALSE);

	SSL_set_bio(sc->ssl, sc->output.bio, sc->input.bio);

	/* either use the server or client part of the protocol */
	if (opt->getInt("debug", FALSE)) {
		if (is_client)
			SSL_set_info_callback(sc->ssl, &info_callback<TRUE>);
		else
			SSL_set_info_callback(sc->ssl, &info_callback<FALSE>);
	}
	if (is_client) {
		SSL_set_connect_state(sc->ssl);
	} else {
		SSL_set_accept_state(sc->ssl);
	}

	if (sc->io == NULL) {
		int result = AObject::from(&sc->io, sc, opt, "async_tcp");
		if (result < 0) {
			TRACE("unknown io module.\n");
			return result;
		}
	}
	return 1;
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
	if (sc->outbuf->length == sc->outbuf->max) {
		BUF_MEM_grow(sc->outbuf, sc->outbuf->length);
	}
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
	AOption *io_opt = option->find("io");
	if (io_opt == NULL) {
		TRACE("require option: \"io\"\n");
		return -EINVAL;
	}

	if (sc->ssl_ctx == NULL) {
		int result = SSL_CTX_create(sc->ssl_ctx, option, TRUE);
		if (result < 0)
			return result;
	}

	int result = SSL_io_init(sc, sc->ssl_ctx, option, TRUE);
	if (result < 0)
		return result;

	sc->input.msg.init(io_opt);
	sc->input.msg.done = &TObjectDone(SSL_IO, input.msg, input.from, SSLOpenStatus);
	sc->input.from = msg;

	sc->state = S_init;
	result = sc->io->open(&sc->input.msg);
	if (result > 0)
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
			assert(0);
		}
		if ((result != SSL_ERROR_WANT_READ) && (result != SSL_ERROR_ZERO_RETURN)) {
			TRACE("SSL_read(%d), error = %d.\n", sc->output.from->size, result);
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
		assert(msg->size == result);

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
	if (msg == NULL)
		return sc->io->shutdown();

	if_not(sc->ssl, NULL, SSL_free);
	return sc->io->close(msg);
}

static int SSLCreate(AObject **object, AObject *parent, AOption *option)
{
	SSL_IO *sc = (SSL_IO*)*object;
	sc->ssl_ctx = NULL; sc->ssl = NULL;
	sc->io = NULL; sc->outbuf = NULL;
	sc->state = S_invalid;
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

struct SSLSvcData : public AObject {
	SSL_CTX  *ssl_ctx;
	AObject  *io_svc_data;
};

static int SSLSvcCreate(AObject **svc_data, AObject *parent, AOption *option)
{
	SSLSvcData *ssl_svc = (SSLSvcData*)*svc_data;
	ssl_svc->ssl_ctx = NULL;
	ssl_svc->io_svc_data = NULL;

	//////////////////////////////////////////////////////////////////////////
	AOption *io_opt = option->find("io");
	if (io_opt == NULL) {
		TRACE("require option: \"io\"\n");
		return -EINVAL;
	}

	IOModule *io = (IOModule*)AModuleFind("io", io_opt->value);
	if ((io == NULL) || (io->svc_accept == NULL)) {
		TRACE("unknown io module(%s), require io->svc_accept().\n", io_opt->value);
		return -EINVAL;
	}

	int result = SSL_CTX_create(ssl_svc->ssl_ctx, option, FALSE);
	if (result < 0)
		return result;

	if (io->svc_module == NULL)
		return 1;
	return AObject::create2(&ssl_svc->io_svc_data, ssl_svc, io_opt, io->svc_module);
}

static void SSLSvcRelease(AObject *svc_data)
{
	SSLSvcData *ssl_svc = (SSLSvcData*)svc_data;
	if_not(ssl_svc->ssl_ctx, NULL, SSL_CTX_free);
	release_s(ssl_svc->io_svc_data);
}

static int SSLSvcAccept(AObject *object, AMessage *msg, AObject *svc_data, AOption *svc_opt)
{
	SSL_IO *sc = (SSL_IO*)object;
	SSLSvcData *ssl_svc = (SSLSvcData*)svc_data;

	int result = SSL_io_init(sc, ssl_svc->ssl_ctx, svc_opt, FALSE);
	if (result < 0)
		return result;

	sc->input.msg.init(msg);
	sc->input.msg.done = &TObjectDone(SSL_IO, input.msg, input.from, SSLOpenStatus);
	sc->input.from = msg;

	sc->state = S_init;
	result = sc->io->m()->svc_accept(sc->io, &sc->input.msg, ssl_svc->io_svc_data, svc_opt->find("io"));
	if (result > 0)
		result = SSLOpenStatus(sc, result);
	return result;
}

AModule SSLSvcModule = {
	"SSLSvcData",
	"SSLSvcData",
	sizeof(SSLSvcData),
	NULL, NULL,
	&SSLSvcCreate,
	&SSLSvcRelease,
};
static int reg_svc = AModuleRegister(&SSLSvcModule);

IOModule OpenSSLModule = { {
	"io",
	"io_openssl",
	sizeof(SSL_IO),
	&SSLInit, &SSLExit,
	&SSLCreate, &SSLRelease, NULL, },
	&SSLOpen,
	&SSLSetOpt, &SSLGetOpt,
	&SSLRequest,
	&SSLCancel,
	&SSLClose,

	&SSLSvcModule,
	&SSLSvcAccept,
};
static int reg_ssl = AModuleRegister(&OpenSSLModule.module);
