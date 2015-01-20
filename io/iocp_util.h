#pragma once
#ifndef _INET_UTIL_H_
#define _INET_UTIL_H_
#endif

#ifndef _WS2TCPIP_H_
#include <WS2tcpip.h>
#endif

extern struct addrinfo*
iocp_getaddrinfo(const char *netaddr, const char *port);

extern int
tcp_connect(SOCKET sock, const struct sockaddr *name, int namelen, int seconds);

static inline int
tcp_send(SOCKET sock, const char *data, int size, int flags)
{
	int left = size;
	while (left > 0) {
		int ret = send(sock, data, left, flags);
		if (ret <= 0)
			return -1;
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
			return -1;
		data += ret;
		left -= ret;
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////
extern int
iocp_connect(SOCKET sock, const struct sockaddr *name, int namelen, WSAOVERLAPPED *ovlp);

extern int
iocp_is_connected(SOCKET sock);

extern int
iocp_send(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp);

extern int
iocp_send(SOCKET sock, const char *data, int size, WSAOVERLAPPED *ovlp);

extern int
iocp_recv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp);

extern int
iocp_recv(SOCKET sock, char *data, int size, WSAOVERLAPPED *ovlp);

//
extern int
iocp_write(HANDLE file, const char *data, int size, OVERLAPPED *ovlp);

extern int
iocp_read(HANDLE file, char *data, int size, OVERLAPPED *ovlp);
