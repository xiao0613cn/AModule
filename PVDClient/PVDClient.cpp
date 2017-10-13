#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/AClientSystem.h"
#include "../ecs/AInOutComponent.h"
#include "PvdNetCmd.h"
#include "md5.h"

struct PVDClient : public AEntity2 {
	AClientComponent _client;
	AInOutComponent _iocom;
	pthread_mutex_t _mutex;
	AOption    *_io_opt;

	PVDStatus   _status;
	uint32_t    _userid;
	uint8_t     _md5id;
	char        _user[NAME_LEN];
	char        _pwd[NAME_LEN];
	int         _last_error;
	STRUCT_SDVR_DEVICE_EX _device2;
	AMessage    _heart_msg;
	pvdnet_head _heart_head;

	void init() {
		AEntity2::init();
		_client.init(this); _push(&_client);
		_iocom.init(this, &_mutex); _push(&_iocom);
		pthread_mutex_init(&_mutex, NULL);
		_io_opt = NULL;

		_status = pvdnet_invalid;
		_userid = _md5id = _last_error = 0;
	}
	int  open(int result);
	void open_prepare(PVDStatus status, int type, int body) {
		_status = status;
		_heart_msg.type = AMsgType_Private|ioMsgType_Block|type;
		_heart_msg.data = _iocom._outbuf->next();
		_heart_msg.size = PVDCmdEncode(_userid, _heart_msg.data, type, body);
	}
};

extern int PVDTryOutput(uint32_t userid, ARefsBuf *&outbuf, AMessage &outmsg)
{
	outmsg.data = outbuf->ptr();
	outmsg.size = outbuf->len();

	int result = PVDCmdDecode(userid, outmsg.data, outmsg.size);
	if (result < 0)
		return result;

	if ((result == 0) || (result > outmsg.size)) {
		if (ARefsBuf::reserve(outbuf, max(result,1024), outbuf->_size) < 0)
			return -ENOMEM;

		outmsg.data = outbuf->ptr();
		if (!ioMsgType_isBlock(outmsg.type))
			return 0;

		if (result == 0) {
			TRACE("%d: reset buffer(%d), drop data(%d).\n",
				userid, outbuf->caps(), outmsg.size);
			outbuf->pop(outbuf->len());
			return 0;
		}
		result = outmsg.size;
	} else {
		outmsg.size = result;
	}
	outbuf->pop(result);
	return result;
}

static void PVDEndInput(AInOutComponent *c, int result)
{
	PVDClient *pvd = container_of(c, PVDClient, _iocom);
	pvd->_last_error = result;
	pvd->_client.use(-1);
}

static int PVDOpenMsgDone(AMessage *msg, int result)
{
	PVDClient *pvd = container_of(msg, PVDClient, _heart_msg);
	result = pvd->open(result);
	if (result != 0)
		pvd->_client.exec_done(result);
	return result;
}
static int PVDOpen(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);
	if (!pvd->_iocom._queue.empty()) {
		assert(0);
		return -EBUSY;
	}
	assert(pvd->_status == pvdnet_invalid);
	pvd->_heart_msg.done = &PVDOpenMsgDone;
	return pvd->open(0);
}
int PVDClient::open(int result)
{
	pvdnet_head *phead;
	do {
	switch (_status)
	{
	case pvdnet_invalid:
		if (_iocom._io == NULL) {
			result = AObject::create(&_iocom._io, this, _io_opt, NULL);
			if (result < 0)
				return result;
		}
		_status = pvdnet_connecting;
		_heart_msg.init(_io_opt);
		result = _iocom._io->open(&_heart_msg);
		break;

	case pvdnet_connecting:
		_userid = 0;
		if (ARefsBuf::reserve(_iocom._outbuf, 2048, 4096) < 0)
			return -ENOMEM;
		_iocom._outbuf->reset();

		open_prepare(pvdnet_syn_md5id, NET_SDVR_MD5ID_GET, 0);
		result = _iocom._io->input(&_heart_msg);
		break;

	case pvdnet_syn_md5id:
		_status = pvdnet_ack_md5id;
		result = _iocom._io->output(&_heart_msg, _iocom._outbuf);
		break;

	case pvdnet_ack_md5id:
		_iocom._outbuf->push(_heart_msg.size);

		result = PVDTryOutput(_userid, _iocom._outbuf, _heart_msg);
		if (result < 0)
			break;
		if (result == 0) {
			result = _iocom._io->output(&_heart_msg, _iocom._outbuf);
			break;
		}

		phead = (pvdnet_head*)_heart_msg.data;
		if ((phead->uCmd != NET_SDVR_MD5ID_GET) || (phead->uResult == 0)) {
			result = -EFAULT;
			break;
		}
		_md5id = *(uint8_t*)(phead+1);

		_status = pvdnet_fin_md5id;
		result = _iocom._io->close(&_heart_msg);
		if (result == 0)
			return 0;

	case pvdnet_fin_md5id:
		_status = pvdnet_reconnecting;
		_heart_msg.init(_io_opt);
		result = _iocom._io->open(&_heart_msg);
		break;

	case pvdnet_reconnecting:
		_iocom._outbuf->reset();
		open_prepare(pvdnet_syn_login, NET_SDVR_LOGIN, sizeof(STRUCT_SDVR_LOGUSER));
	{
		STRUCT_SDVR_LOGUSER *login = (STRUCT_SDVR_LOGUSER*)(_heart_msg.data+sizeof(pvdnet_head));
		memset(login, 0, sizeof(*login));

		strcpy_sz(login->szUserName, _user);
		MD5_enc(_md5id, (uint8_t*)_pwd, strlen(_pwd), (uint8_t*)login->szPassWord);

		login->dwNamelen = strlen(login->szUserName);
		login->dwPWlen = PASSWD_LEN;

		_status = pvdnet_syn_login;
		result = _iocom._io->input(&_heart_msg);
		break;
	}
	case pvdnet_syn_login:
		_status = pvdnet_ack_login;
		result = _iocom._io->output(&_heart_msg, _iocom._outbuf);
		break;

	case pvdnet_ack_login:
		_iocom._outbuf->push(_heart_msg.size);

		result = PVDTryOutput(_userid, _iocom._outbuf, _heart_msg);
		if (result < 0)
			break;
		if (result == 0) {
			result = _iocom._io->output(&_heart_msg, _iocom._outbuf);
			break;
		}

		phead = (pvdnet_head*)_heart_msg.data;
		if ((phead->uCmd != NET_SDVR_LOGIN) || (phead->uResult == 0)) {
			result = -EFAULT;
			break;
		}

		result -= sizeof(pvdnet_head);
		memcpy(&_device2, phead+1, min(sizeof(_device2), result));
		_userid = phead->uUserId;

		if (result == sizeof(STRUCT_SDVR_DEVICE_EX))
			_status = pvdnet_con_devinfo2;
		else if (result == sizeof(STRUCT_SDVR_DEVICE))
			_status = pvdnet_con_devinfo;
		else
			_status = pvdnet_con_devinfox;
		_last_error = result;

		_client.use(2);
		_iocom._input_begin(&PVDEndInput);
		_iocom._output_begin(2048, 4096);
		return result;

	default: assert(0); return -EACCES;
	}
	} while (result > 0);
	return result;
}

static int PVDHeartMsgDone(AMessage *msg, int result)
{
	PVDClient *pvd = container_of(msg, PVDClient, _heart_msg);
	pvd->_client.exec_done(result);
	return result;
}
static int PVDHeart(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);

	pvd->_heart_msg.init(AMsgType_Private|NET_SDVR_SHAKEHAND, &pvd->_heart_head, 0);
	pvd->_heart_msg.size = PVDCmdEncode(pvd->_userid, &pvd->_heart_head, NET_SDVR_SHAKEHAND, 0);
	pvd->_heart_msg.done = &PVDHeartMsgDone;

	pvd->_iocom.post(&pvd->_heart_msg);
	return 0;
}

static int PVDInput(AInOutComponent *c, AMessage *msg)
{
	PVDClient *pvd = container_of(c, PVDClient, _iocom);
	if (msg->type & AMsgType_Private)
	{
		pvdnet_head *phead = (pvdnet_head*)msg->data;
		if (phead->uFlag != NET_CMD_HEAD_FLAG) {
			assert(0);
			return -EINVAL;
		}
		phead->uUserId = pvd->_userid;
	}
	c->_inmsg.init(msg);
	return c->_io->input(&c->_inmsg);
}

static int PVDOutput(AInOutComponent *c, int result)
{
	PVDClient *pvd = container_of(c, PVDClient, _iocom);
	if (result >= 0)
		result = PVDTryOutput(pvd->_userid, pvd->_iocom._outbuf, pvd->_iocom._outmsg);
	if (result == 0) // need more data
		return 0;

	if (pvd->_iocom._abort || (result < 0)) {
		pvd->_client._main_abort = true;
		pvd->_client.use(-1);
		pvd->_iocom._input_end(result);
		pvd->_iocom._output_end();
		return -EINTR;
	}

	// TODO: dispatch outmsg
	pvdnet_head *phead = (pvdnet_head*)pvd->_iocom._outmsg.data;
	TRACE("pvd(%p): recv msg = 0x%02X, size = %d.\n",
		pvd, phead->uCmd, result);
	return result;
}

static int PVDAbort(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);
	pvd->_iocom._input_end(-EINTR);

	if (pvd->_iocom._io != NULL)
		pvd->_iocom._io->shutdown();
	return 1;
}

static int PVDCloseMsgDone(AMessage *msg, int result)
{
	PVDClient *pvd = container_of(msg, PVDClient, _heart_msg);
	pvd->_client.exec_done(result);
	return result;
}
static int PVDClose(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);
	if (pvd->_iocom._io == NULL)
		return 1;

	pvd->_status = pvdnet_invalid;
	pvd->_heart_msg.init();
	pvd->_heart_msg.done = &PVDCloseMsgDone;
	return pvd->_iocom._io->close(&pvd->_heart_msg);
}

static int PVDCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDClient *pvd = (PVDClient*)*object;
	pvd->init();

	pvd->_io_opt = option->find("io");
	if (pvd->_io_opt != NULL)
		pvd->_io_opt = AOptionClone(pvd->_io_opt, NULL);

	strcpy_sz(pvd->_user, option->getStr("username", ""));
	strcpy_sz(pvd->_pwd, option->getStr("password", ""));
	pvd->_client._tick_heart = option->getInt("tick_heart", 3)*1000;

	pvd->_client.open = &PVDOpen;
	pvd->_client.heart = &PVDHeart;
	pvd->_client.abort = &PVDAbort;
	pvd->_client.close = &PVDClose;
	pvd->_iocom.do_input = &PVDInput;
	pvd->_iocom.on_output = &PVDOutput;
	return 1;
}

static void PVDRelease(AObject *object)
{
	PVDClient *pvd = (PVDClient*)object;
	pvd->_pop(&pvd->_client);
	pvd->_pop(&pvd->_iocom);
	release_s(pvd->_iocom._io);
	release_s(pvd->_iocom._outbuf);

	pthread_mutex_destroy(&pvd->_mutex);
	release_s(pvd->_io_opt);
	pvd->exit();
}

AModule PVDClientModule = {
	"AEntity2",
	"PVDClient",
	sizeof(PVDClient),
	NULL, NULL,
	&PVDCreate,
	&PVDRelease,
};

static auto_reg_t reg(PVDClientModule);
