#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"
#include "../ecs/AClientSystem.h"
#include "../ecs/AInOutComponent.h"
#include "PvdNetCmd.h"
#include "md5.h"

struct PVDClient : public AEntity2 {
	AClientComponent _client;
	AInOutComponent _io_com;
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
		_io_com.init(this); _push(&_io_com);
		pthread_mutex_init(&_mutex, NULL);
		_io_com._mutex = &_mutex;
		_io_opt = NULL;

		_status = pvdnet_invalid;
		_userid = _md5id = _last_error = 0;
		_heart_msg.done = NULL;
	}
	int  open(int result);
	void open_prepare(PVDStatus status, int type, int body) {
		_status = status;
		_io_com._outmsg.type = AMsgType_Private|ioMsgType_Block|type;
		_io_com._outmsg.data = _io_com._outbuf->next();
		_io_com._outmsg.size = PVDCmdEncode(_userid, _io_com._outmsg.data, type, body);
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
		if (ARefsBufCheck(outbuf, max(result,1024), outbuf->size) < 0)
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
	if (ARefsBufCheck(outbuf, 1024, outbuf->size) < 0)
		return -ENOMEM;
	return result;
}

static int PVDOpenMsgDone(AMessage *msg, int result)
{
	PVDClient *pvd = container_of(msg, PVDClient, _io_com._outmsg);
	result = pvd->open(result);
	if (result != 0)
		pvd->_client.exec_done(result);
	return result;
}
static int PVDOpen(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);
	if (!pvd->_io_com._queue.empty()) {
		assert(0);
		return -EBUSY;
	}
	assert(pvd->_status == pvdnet_invalid);
	pvd->_io_com._outmsg.done = &PVDOpenMsgDone;
	return pvd->open(0);
}
int PVDClient::open(int result)
{
	pvdnet_head *phead;
	do {
	switch (_status)
	{
	case pvdnet_invalid:
		if (_io_com._io == NULL) {
			result = AObject::create(&_io_com._io, this, _io_opt, NULL);
			if (result < 0)
				return result;
		}
		_status = pvdnet_connecting;
		_io_com._outmsg.init(_io_opt);
		result = _io_com._io->open(&_io_com._outmsg);
		break;

	case pvdnet_connecting:
		_userid = 0;
		if (ARefsBufCheck(_io_com._outbuf, 2048, 4096) < 0)
			return -ENOMEM;

		open_prepare(pvdnet_syn_md5id, NET_SDVR_MD5ID_GET, 0);
		result = _io_com._io->input(&_io_com._outmsg);
		break;

	case pvdnet_syn_md5id:
		_status = pvdnet_ack_md5id;
		result = _io_com._io->output(&_io_com._outmsg, _io_com._outbuf);
		break;

	case pvdnet_ack_md5id:
		_io_com._outbuf->push(_io_com._outmsg.size);

		result = PVDTryOutput(_userid, _io_com._outbuf, _io_com._outmsg);
		if (result < 0)
			return result;
		if (result == 0) {
			result = _io_com._io->output(&_io_com._outmsg, _io_com._outbuf);
			break;
		}

		phead = (pvdnet_head*)_io_com._outmsg.data;
		if ((phead->uCmd != NET_SDVR_MD5ID_GET) || (phead->uResult == 0)) {
			result = -EFAULT;
			break;
		}
		_md5id = *(uint8_t*)(phead+1);

		_status = pvdnet_fin_md5id;
		result = _io_com._io->close(&_io_com._outmsg);
		if (result == 0)
			return 0;

	case pvdnet_fin_md5id:
		_status = pvdnet_reconnecting;
		_io_com._outmsg.init(_io_opt);
		result = _io_com._io->open(&_io_com._outmsg);
		break;

	case pvdnet_reconnecting:
		_io_com._outbuf->reset();
		open_prepare(pvdnet_syn_login, NET_SDVR_LOGIN, sizeof(STRUCT_SDVR_LOGUSER));
	{
		STRUCT_SDVR_LOGUSER *login = (STRUCT_SDVR_LOGUSER*)(_io_com._outmsg.data+sizeof(pvdnet_head));
		memset(login, 0, sizeof(*login));

		strcpy_sz(login->szUserName, _user);
		MD5_enc(_md5id, (uint8_t*)_pwd, strlen(_pwd), (uint8_t*)login->szPassWord);

		login->dwNamelen = strlen(login->szUserName);
		login->dwPWlen = PASSWD_LEN;

		_status = pvdnet_syn_login;
		result = _io_com._io->input(&_io_com._outmsg);
		break;
	}
	case pvdnet_syn_login:
		_status = pvdnet_ack_login;
		result = _io_com._io->output(&_io_com._outmsg, _io_com._outbuf);
		break;

	case pvdnet_ack_login:
		_io_com._outbuf->push(_io_com._outmsg.size);

		result = PVDTryOutput(_userid, _io_com._outbuf, _io_com._outmsg);
		if (result < 0)
			break;

		if (result == 0) {
			result = _io_com._io->output(&_io_com._outmsg, _io_com._outbuf);
			break;
		}

		phead = (pvdnet_head*)_io_com._outmsg.data;
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

		addref();
		_client.use(2);
		_io_com._abort = false;
		_io_com._outmsg_cycle(2048, 4096);
		return result;

	default: return -EACCES;
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

	pvd->_io_com.post(&pvd->_heart_msg);
	return 0;
}

static void PVDInput(AInOutComponent *c, AMessage *msg)
{
	PVDClient *pvd = container_of(c, PVDClient, _io_com);
	if (msg->type & AMsgType_Private) {
		pvdnet_head *phead = (pvdnet_head*)msg->data;
		phead->uFlag = NET_CMD_HEAD_FLAG;
		phead->uUserId = pvd->_userid;
	}
	AInOutComponent::_do_input(c, msg);
}

static void PVDEndQueue(AInOutComponent *c, int result)
{
	PVDClient *pvd = container_of(c, PVDClient, _io_com);
	pvd->_client.use(-1);
}

static int PVDOutput(AInOutComponent *c, int result)
{
	PVDClient *pvd = container_of(c, PVDClient, _io_com);
	if (result >= 0)
		result = PVDTryOutput(pvd->_userid, pvd->_io_com._outbuf, pvd->_io_com._outmsg);
	if (result == 0) // need more data
		return 0;

	if (result < 0) {
		pvd->_client._main_abort = true;
		pvd->_client.use(-1);

		// abort input
		c->lock();
		c->_abort = true;
		if (c->_queue.empty())
			PVDEndQueue(c, result);
		c->unlock();

		pvd->release();
		return result;
	}

	// TODO: dispatch outmsg
	pvdnet_head *phead = (pvdnet_head*)pvd->_io_com._outmsg.data;
	TRACE("pvd(%p): recv msg = 0x%02X, size = %d.\n",
		pvd, phead->uCmd, result);
	return 0;
}

static int PVDAbort(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);
	if (pvd->_io_com._io != NULL)
		pvd->_io_com._io->shutdown();
	return 1;
}

static int PVDCloseMsgDone(AMessage *msg, int result)
{
	PVDClient *pvd = container_of(msg, PVDClient, _io_com._outmsg);
	pvd->_client.exec_done(result);
	return result;
}
static int PVDClose(AClientComponent *c)
{
	PVDClient *pvd = container_of(c, PVDClient, _client);
	if (pvd->_io_com._io == NULL)
		return 1;

	pvd->_status = pvdnet_invalid;
	pvd->_io_com._outmsg.init();
	pvd->_io_com._outmsg.done = &PVDCloseMsgDone;
	return pvd->_io_com._io->close(&pvd->_io_com._outmsg);
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
	pvd->_io_com.do_input = &PVDInput;
	pvd->_io_com.on_endqu = &PVDEndQueue;
	pvd->_io_com.on_output = &PVDOutput;
	return 1;
}

static void PVDRelease(AObject *object)
{
	PVDClient *pvd = (PVDClient*)object;
	pvd->_pop(&pvd->_client);
	pvd->_pop(&pvd->_io_com);
	release_s(pvd->_io_com._io, AObjectRelease, NULL);
	release_s(pvd->_io_com._outbuf, ARefsBufRelease, NULL);

	pthread_mutex_destroy(&pvd->_mutex);
	release_s(pvd->_io_opt, AOptionRelease, NULL);
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
