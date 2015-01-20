#include "stdafx.h"
#include "iocp_util.h"

#include <MSWSock.h>
#pragma comment(lib, "ws2_32.lib")


struct addrinfo*
iocp_getaddrinfo(const char *netaddr, const char *port)
{
	char ipaddr[MAX_PATH];
	if (port != NULL) {
		strcpy_s(ipaddr, netaddr);
		if (port == INVALID_HANDLE_VALUE)
			port = NULL;
	} else if ((port = strchr(netaddr, ':')) != NULL) {
		strncpy_s(ipaddr, sizeof(ipaddr), netaddr, port-netaddr);
		port += 1;
	} else {
		strcpy_s(ipaddr, netaddr);
	}

	struct addrinfo ai;
	memset(&ai, 0, sizeof(ai));
	ai.ai_flags    = AI_NUMERICSERV;
	ai.ai_family   = AF_UNSPEC;
	ai.ai_socktype = 0;
	ai.ai_protocol = 0;

	struct addrinfo *res = NULL;
	getaddrinfo(ipaddr, port, &ai, &res);
	return res;
}

int
tcp_connect(SOCKET sock, const struct sockaddr *name, int namelen, int seconds)
{
	u_long nonblocking = 1;
	int ret = ioctlsocket(sock, FIONBIO, &nonblocking);
	if (ret == SOCKET_ERROR)
		return SOCKET_ERROR;

	ret = connect(sock, name, namelen);

	// timeout checking
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;

	struct fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(sock, &wfds);

	ret = select(sock, NULL, &wfds, NULL, &tv);
	if (ret == SOCKET_ERROR)
		return SOCKET_ERROR;
	if (ret == 0)
		return SOCKET_ERROR;

	// error checking
	int error = 0;
	int errorlen = sizeof(error);
	ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &errorlen);
	if (ret == SOCKET_ERROR)
		return SOCKET_ERROR;
	if (error != 0)
		return SOCKET_ERROR;

	// reset to blocking io
	nonblocking = 0;
	ret = ioctlsocket(sock, FIONBIO, &nonblocking);
	if (ret == SOCKET_ERROR)
		return SOCKET_ERROR;

	//success
	return 0;
}

//////////////////////////////////////////////////////////////////////////
int
iocp_connect(SOCKET sock, const struct sockaddr *name, int namelen, WSAOVERLAPPED *ovlp)
{
	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = name->sa_family;
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(ADDR_ANY);
	int ret = bind(sock, (const sockaddr*)&addr, sizeof(addr));
	if (ret != 0)
		return SOCKET_ERROR;

	DWORD tx;
	LPFN_CONNECTEX ConnectEx = NULL;
	GUID ConnectEx_GUID = WSAID_CONNECTEX;

	ret = WSAIoctl(
		sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&ConnectEx_GUID, sizeof(ConnectEx_GUID), &ConnectEx, sizeof(ConnectEx),
		&tx, NULL, NULL);
	if (ret != 0)
		return SOCKET_ERROR;

	ret = ConnectEx(sock, name, namelen, NULL, 0, NULL, ovlp);
	if (ret) {
		ret = 0;
	} else if ((ret = WSAGetLastError()) == WSA_IO_PENDING) {
		ret = 0;
	} else {
		ret = SOCKET_ERROR;
	}
	return ret;
}

int
iocp_is_connected(SOCKET sock)
{
	int ret = setsockopt(sock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	if (ret == SOCKET_ERROR) {
		return SOCKET_ERROR;
	}

	DWORD seconds;
	int size = sizeof(seconds);

	ret = getsockopt(sock, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, (int*)&size);
	if ((ret != SOCKET_ERROR) && (seconds == 0xffffffff)) {
		ret = SOCKET_ERROR;
	}
	return ret;
}

int
iocp_send(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp)
{
	DWORD tx = 0;
	DWORD flag = 0;

	int ret = WSASend(sock, buffer, count, &tx, flag, ovlp, NULL);
	if (ret != 0)
	{
		ret = WSAGetLastError();
		if (ret == WSA_IO_PENDING)
			ret = 0;
		else
			ret = SOCKET_ERROR;
	}
	return ret;
}

int
iocp_send(SOCKET sock, const char *data, int size, WSAOVERLAPPED *ovlp)
{
	WSABUF buffer;
	buffer.buf = (char*)data;
	buffer.len = size;

	return iocp_send(sock, &buffer, 1, ovlp);
}

int
iocp_recv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp)
{
	DWORD tx = 0;
	DWORD flag = 0;

	int ret = WSARecv(sock, buffer, count, &tx, &flag, ovlp, NULL);
	if (ret != 0)
	{
		ret = WSAGetLastError();
		if (ret == WSA_IO_PENDING)
			ret = 0;
		else
			ret = SOCKET_ERROR;
	}
	return ret;
}

int
iocp_recv(SOCKET sock, char *data, int size, WSAOVERLAPPED *ovlp)
{
	WSABUF buffer;
	buffer.buf = data;
	buffer.len = size;

	return iocp_recv(sock, &buffer, 1, ovlp);
}

//
int
iocp_write(HANDLE file, const char *data, int size, OVERLAPPED *ovlp)
{
	DWORD tx = 0;
	int ret = WriteFile(file, data, size, &tx, ovlp);
	if (ret) {
		ret = 0;
	} else {
		ret = GetLastError();
		if (ret == ERROR_IO_PENDING)
			ret = 0;
		else
			ret = -1;
	}
	return ret;
}

int
iocp_read(HANDLE file, char *data, int size, OVERLAPPED *ovlp)
{
	DWORD tx = 0;
	int ret = ReadFile(file, data, size, &tx, ovlp);
	if (ret) {
		ret = 0;
	} else {
		ret = GetLastError();
		if (ret == ERROR_IO_PENDING)
			ret = 0;
		else
			ret = -1;
	}
	return ret;
}
