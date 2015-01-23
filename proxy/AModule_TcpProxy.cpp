#include "stdafx.h"
#include "../base/AModule.h"
#include "../io/iocp_util.h"


static SOCKET proxy_socket = INVALID_SOCKET;

static DWORD WINAPI PVDProxyProcess(void *p)
{
	SOCKET sock = (SOCKET)p;
	AOption opt;

	AObject *tcp = NULL;
	long result = AObjectCreate(&tcp, NULL, NULL, "tcp");
	if (result >= 0) {
		AOptionInit(&opt, NULL);
		strcpy_s(opt.name, "socket");
		opt.extend = p;

		result = tcp->setopt(tcp, &opt);
		if (result >= 0)
			sock = NULL;
	}

	AObject *pvd = NULL;
	if (result >= 0) {
		AOptionInit(&opt, NULL);
		strcpy_s(opt.name, "PVDClient");

		extern AModule SyncControlModule;
		result = SyncControlModule.create(&pvd, NULL, opt);
	}
	if (result >= 0) {
		AOptionInit(&opt, NULL);
		strcpy_s(opt.name, "io");
		opt.extend = tcp;

		result = pvd->setopt(pvd, &opt);
	}

	if (result >= 0) {
	}
}

static DWORD WINAPI PVDProxyService(void*)
{
	struct sockaddr addr;
	int addrlen;
	SOCKET sock;

	do {
		memset(&addr, 0, sizeof(addr));
		addrlen = sizeof(addr);

		sock = accept(proxy_socket, &addr, &addrlen);
		if (sock == INVALID_SOCKET)
			break;

		QueueUserWorkItem(&PVDProxyProcess, sock, 0);
	} while (1);
	return 0;
}

static long PVDProxyInit(void)
{
	proxy_socket = bind_socket(IPPROTO_TCP, 8101);
	if (proxy_socket == INVALID_SOCKET)
		return -EIO;

	QueueUserWorkItem(&PVDProxyService, NULL, 0);
	return 1;
}

static void PVDProxyExit(void)
{
	if (proxy_socket == NULL)
		return;

	closesocket(proxy_socket);
	proxy_socket = NULL;
}

//////////////////////////////////////////////////////////////////////////
struct PVDProxy {
	AObject  object;
	AObject *pvd;
};
#define to_pvd(obj) container_of(obj, PVDProxy, object)

static void PVDProxyRelease(AObject *object)
{
	PVDProxy *pvd = to_pvd(object);
	release_s(pvd->io, AObjectRelease, NULL);
	free(pvd);
}

static long PVDProxyCreate(AObject **object, AObject *parent, AOption *option)
{
	PVDProxy *pvd = (PVDProxy*)malloc(sizeof(PVDProxy));
	if (pvd == NULL)
		return -ENOMEM;

	extern AModule PVDProxyModule;
	AObjectInit(&pvd->object, &PVDProxyModule);
	pvd->io = NULL;

	AOption *io_opt = AOptionFindChild(option, "io");
	long result = AObjectCreate(&pvd->io, pvd, io_opt, "tcp");

	*object = &pvd->object;
	return result;
}

