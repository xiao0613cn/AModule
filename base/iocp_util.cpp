#include "stdafx.h"
#include "AModule_API.h"

#ifdef _WIN32
#ifndef _MSWSOCK_
#include <MSWSock.h>
#endif
#pragma comment(lib, "ws2_32.lib")
#endif

AMODULE_API struct addrinfo*
tcp_getaddrinfo(const char *netaddr, const char *port)
{
	char ipaddr[BUFSIZ];
	if (port != NULL) {
		strcpy_s(ipaddr, netaddr);
		if (port == (void*)-1)
			port = NULL;
	} else if ((port = strchr(netaddr, ':')) != NULL) {
		int len = min(sizeof(ipaddr)-1,port-netaddr);
		strncpy(ipaddr, netaddr, len); ipaddr[len] = '\0';
		port += 1;
	} else {
		strcpy_s(ipaddr, netaddr);
	}

	struct addrinfo ai;
	memset(&ai, 0, sizeof(ai));
	ai.ai_flags    = 0;
	ai.ai_family   = AF_UNSPEC;
	ai.ai_socktype = 0;
	ai.ai_protocol = 0;

	struct addrinfo *res = NULL;
	getaddrinfo(ipaddr, port, &ai, &res);
	return res;
}

AMODULE_API SOCKET
tcp_bind(int family, int protocol, unsigned short port)
{
	int type;
	if (protocol == IPPROTO_TCP)
		type = SOCK_STREAM;
	else
		type = SOCK_DGRAM;

	SOCKET sock = socket(family, type, protocol);
	if (sock == INVALID_SOCKET)
		return INVALID_SOCKET;

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	int result = bind(sock, (const sockaddr*)&addr, sizeof(sockaddr_in));
	if (result != 0) {
		closesocket(sock);
		return INVALID_SOCKET;
	}
	return sock;
}

AMODULE_API int
tcp_connect(SOCKET sock, const struct sockaddr *name, int namelen, int seconds)
{
	if (tcp_nonblock(sock, 1) != 0)
		return -EIO;

	int ret = connect(sock, name, namelen);

	// timeout checking
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;

	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(sock, &wfds);

	ret = select(sock+1, NULL, &wfds, NULL, &tv);
	if (ret <= 0)
		return -EAGAIN;

	// error checking
	int error = 0;
	socklen_t errorlen = sizeof(error);
	ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &errorlen);
	if ((ret != 0) || (error != 0))
		return -EIO;

	// reset to blocking io
	if (tcp_nonblock(sock, 0) != 0)
		return -EIO;

	//success
	return 1;
}

AMODULE_API int
tcp_nonblock(SOCKET sock, u_long nonblocking)
{
#ifdef _WIN32
	return ioctlsocket(sock, FIONBIO, &nonblocking);
#else
	int flags;
	if ((flags = fcntl(sock, F_GETFL, NULL)) < 0) {
		//event_warn("fcntl(%d, F_GETFL)", fd);
		return -1;
	}

	if (nonblocking)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(sock, F_SETFL, flags) == -1) {
		//event_warn("fcntl(%d, F_SETFL)", fd);
		return -1;
	}
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
AMODULE_API int
iocp_connect(SOCKET sock, const struct sockaddr *name, int namelen, WSAOVERLAPPED *ovlp)
{
	SOCKADDR_IN addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = name->sa_family;
	addr.sin_port        = htons(0);
	addr.sin_addr.s_addr = htonl(ADDR_ANY);
	int ret = bind(sock, (const sockaddr*)&addr, sizeof(addr));
	if (ret != 0)
		return -EIO;

	DWORD tx;
	LPFN_CONNECTEX ConnectEx = NULL;
	GUID ConnectEx_GUID = WSAID_CONNECTEX;

	ret = WSAIoctl(
		sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&ConnectEx_GUID, sizeof(ConnectEx_GUID), &ConnectEx, sizeof(ConnectEx),
		&tx, NULL, NULL);
	if (ret != 0)
		return -EIO;

	ret = ConnectEx(sock, name, namelen, NULL, 0, NULL, ovlp);
	if (!ret && (WSAGetLastError() != WSA_IO_PENDING))
		return -EIO;

	return 0;
}

AMODULE_API int
iocp_is_connected(SOCKET sock)
{
	int ret = setsockopt(sock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
	if (ret != 0)
		return -EIO;

	DWORD seconds;
	int size = sizeof(seconds);

	ret = getsockopt(sock, SOL_SOCKET, SO_CONNECT_TIME, (char*)&seconds, (int*)&size);
	if ((ret != 0) || (seconds == 0xffffffff))
		return -EIO;

	return 1;
}

AMODULE_API int
iocp_sendv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp)
{
	DWORD tx = 0;
	DWORD flag = 0;

	int ret = WSASend(sock, buffer, count, &tx, flag, ovlp, NULL);
	if ((ret != 0) && (WSAGetLastError() != WSA_IO_PENDING))
		return -EIO;

	return 0;
}

AMODULE_API int
iocp_send(SOCKET sock, const char *data, int size, WSAOVERLAPPED *ovlp)
{
	WSABUF buffer;
	buffer.buf = (char*)data;
	buffer.len = size;

	return iocp_sendv(sock, &buffer, 1, ovlp);
}

AMODULE_API int
iocp_recvv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp)
{
	DWORD tx = 0;
	DWORD flag = 0;

	int ret = WSARecv(sock, buffer, count, &tx, &flag, ovlp, NULL);
	if ((ret != 0) && (WSAGetLastError() != WSA_IO_PENDING))
		return -EIO;

	return 0;
}

AMODULE_API int
iocp_recv(SOCKET sock, char *data, int size, WSAOVERLAPPED *ovlp)
{
	WSABUF buffer;
	buffer.buf = data;
	buffer.len = size;

	return iocp_recvv(sock, &buffer, 1, ovlp);
}

//
AMODULE_API int
iocp_write(HANDLE file, const char *data, int size, OVERLAPPED *ovlp)
{
	DWORD tx = 0;
	BOOL ret = WriteFile(file, data, size, &tx, ovlp);
	if (!ret && (GetLastError() != ERROR_IO_PENDING))
		return -EIO;

	return 0;
}

AMODULE_API int
iocp_read(HANDLE file, char *data, int size, OVERLAPPED *ovlp)
{
	DWORD tx = 0;
	BOOL ret = ReadFile(file, data, size, &tx, ovlp);
	if (!ret && (GetLastError() != ERROR_IO_PENDING))
		return -EIO;

	return 0;
}
#endif
