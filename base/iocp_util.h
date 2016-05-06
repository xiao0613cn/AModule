#ifndef _IOCP_UTIL_H_
#define _IOCP_UTIL_H_

#ifndef _WS2TCPIP_H_
#include <WS2tcpip.h>
#endif

AMODULE_API struct addrinfo*
iocp_getaddrinfo(const char *netaddr, const char *port);

AMODULE_API SOCKET
bind_socket(int family, int protocol, unsigned short port);

AMODULE_API int
tcp_connect(SOCKET sock, const struct sockaddr *name, int namelen, int seconds);

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
AMODULE_API int
iocp_connect(SOCKET sock, const struct sockaddr *name, int namelen, WSAOVERLAPPED *ovlp);

AMODULE_API int
iocp_is_connected(SOCKET sock);

AMODULE_API int
iocp_sendv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp);

AMODULE_API int
iocp_send(SOCKET sock, const char *data, int size, WSAOVERLAPPED *ovlp);

AMODULE_API int
iocp_recvv(SOCKET sock, WSABUF *buffer, int count, WSAOVERLAPPED *ovlp);

AMODULE_API int
iocp_recv(SOCKET sock, char *data, int size, WSAOVERLAPPED *ovlp);

//
AMODULE_API int
iocp_write(HANDLE file, const char *data, int size, OVERLAPPED *ovlp);

AMODULE_API int
iocp_read(HANDLE file, char *data, int size, OVERLAPPED *ovlp);

#endif
