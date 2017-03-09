#ifndef _IOCP_UTIL_H_
#define _IOCP_UTIL_H_

#ifdef _WIN32
#ifndef _WS2TCPIP_H_
#include <WS2tcpip.h>
#endif
#else

#endif

AMODULE_API struct addrinfo*
tcp_getaddrinfo(const char *netaddr, const char *port);

AMODULE_API SOCKET
tcp_bind(int family, int protocol, unsigned short port);

AMODULE_API int
tcp_connect(SOCKET sock, const struct sockaddr *name, int namelen, int seconds);

AMODULE_API int
tcp_nonblock(SOCKET sock, u_long nonblocking);

static inline int
tcp_send(SOCKET sock, const char *data, int size, int flags)
{
	int left = size;
	while (left > 0) {
		int ret = send(sock, data, left, flags);
		if (ret <= 0)
			return -EIO;
		data += ret;
		left -= ret;
	}
	return size;
}

static inline int
tcp_recv(SOCKET sock, char *data, int size, int flags)
{
	int left = size;
	while (left > 0) {
		int ret = recv(sock, data, left, flags);
		if (ret <= 0)
			return -EIO;
		data += ret;
		left -= ret;
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
AMODULE_API int
iocp_connect(SOCKET sock, const struct sockaddr *name, int namelen, WSAOVERLAPPED *ovlp);

AMODULE_API int
iocp_is_connected(SOCKET sock);

static inline int
iocp_sendv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp)
{
	DWORD tx = 0;
	DWORD flag = 0;

	int ret = WSASend(sock, buffer, count, &tx, flag, ovlp, NULL);
	if ((ret != 0) && ((tx=WSAGetLastError()) != WSA_IO_PENDING))
		return -EIO;

	return 0;
}

static inline int
iocp_send(SOCKET sock, const char *data, int size, WSAOVERLAPPED *ovlp)
{
	WSABUF buffer;
	buffer.buf = (char*)data;
	buffer.len = size;

	return iocp_sendv(sock, &buffer, 1, ovlp);
}

static inline int
iocp_recvv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp)
{
	DWORD tx = 0;
	DWORD flag = 0;

	int ret = WSARecv(sock, buffer, count, &tx, &flag, ovlp, NULL);
	if ((ret != 0) && ((tx=WSAGetLastError()) != WSA_IO_PENDING))
		return -EIO;

	return 0;
}

static inline int
iocp_recv(SOCKET sock, char *data, int size, WSAOVERLAPPED *ovlp)
{
	WSABUF buffer;
	buffer.buf = data;
	buffer.len = size;

	return iocp_recvv(sock, &buffer, 1, ovlp);
}

//
static inline int
iocp_write(HANDLE file, const char *data, int size, OVERLAPPED *ovlp)
{
	DWORD tx = 0;
	BOOL ret = WriteFile(file, data, size, &tx, ovlp);
	if (!ret && ((tx=GetLastError()) != ERROR_IO_PENDING))
		return -EIO;

	return 0;
}

static inline int
iocp_read(HANDLE file, char *data, int size, OVERLAPPED *ovlp)
{
	DWORD tx = 0;
	BOOL ret = ReadFile(file, data, size, &tx, ovlp);
	if (!ret && ((tx=GetLastError()) != ERROR_IO_PENDING))
		return -EIO;

	return 0;
}
#endif

#endif
