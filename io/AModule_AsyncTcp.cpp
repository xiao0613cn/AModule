#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"

enum op_status {
	op_none = 0,
	op_pending,
	op_signal,
	op_error,
};

struct AsyncTcp;
struct AsyncOvlp {
	AOperator sysio;
	AsyncTcp *tcp;
	AMessage *msg;
#ifdef _WIN32
	WSABUF    buf;
#else
	int       pos;
	unsigned int  perform_count;
	unsigned int  signal_count;
	long volatile status;
#endif
};
struct AsyncTcp {
	AObject   object;
	SOCKET    sock;
	AsyncOvlp send_ovlp;
	AsyncOvlp recv_ovlp;
};
#define to_tcp(obj)  container_of(obj, AsyncTcp, object)

static void AsyncTcpRelease(AObject *object)
{
	AsyncTcp *tcp = to_tcp(object);
	release_s(tcp->sock, closesocket, INVALID_SOCKET);
}

static int AsyncTcpCreate(AObject **object, AObject *parent, AOption *option)
{
	AsyncTcp *tcp = (AsyncTcp*)*object;
	tcp->sock = INVALID_SOCKET;

	memset(&tcp->send_ovlp, 0, sizeof(tcp->send_ovlp));
	memset(&tcp->recv_ovlp, 0, sizeof(tcp->recv_ovlp));
	tcp->send_ovlp.tcp = tcp;
	tcp->recv_ovlp.tcp = tcp;
	return 1;
}

#ifdef _WIN32
static void AsyncTcpRequestDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = container_of(asop, AsyncOvlp, sysio);

	if (result <= 0) {
		result = -EIO;
		result = ovlp->msg->done(ovlp->msg, -EIO);
		return;
	}

	if (!ioMsgType_isBlock(ovlp->msg->type)) {
		ovlp->msg->size = result;
		result = ovlp->msg->done(ovlp->msg, result);
		return;
	}

	ovlp->buf.buf += result;
	ovlp->buf.len -= result;

	if (ovlp->buf.len == 0) {
		result = ovlp->msg->size;
	} else {
		AsyncTcp *tcp = ovlp->tcp;
		if (ovlp == &tcp->send_ovlp)
			result = iocp_sendv(tcp->sock, &ovlp->buf, 1, &ovlp->sysio.ao_ovlp);
		else
			result = iocp_recvv(tcp->sock, &ovlp->buf, 1, &ovlp->sysio.ao_ovlp);
		if (result == 0)
			return;
	}
	result = ovlp->msg->done(ovlp->msg, result);
}

static void AsyncTcpOpenDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = container_of(asop, AsyncOvlp, sysio);
	AsyncTcp *tcp = ovlp->tcp;

	if (result >= 0)
		result = iocp_is_connected(tcp->sock);

	tcp->send_ovlp.sysio.callback = &AsyncTcpRequestDone;
	tcp->recv_ovlp.sysio.callback = &AsyncTcpRequestDone;
	result = ovlp->msg->done(ovlp->msg, result);
}
#else
static int AsyncOvlpProc(AsyncTcp *tcp, AsyncOvlp *ovlp)
{
	int result;
_retry:
	result = InterlockedCompareExchange(&ovlp->status, op_none, op_signal);
	if (result == op_error)
		return -EIO;

	if ((result == op_signal) || (ovlp->perform_count != ovlp->signal_count)) {
		if (ovlp == &tcp->send_ovlp) {
			result = send(tcp->sock, ovlp->msg->data+ovlp->pos, ovlp->msg->size-ovlp->pos, MSG_NOSIGNAL);
		} else {
			result = recv(tcp->sock, ovlp->msg->data+ovlp->pos, ovlp->msg->size-ovlp->pos, 0);
		}
		if (result == 0)
			return -EIO;

		if (result > 0) {
			ovlp->pos += result;
			if (ovlp->pos == ovlp->msg->size)
				return ovlp->pos;

			ovlp->perform_count += 1;
			if (!ioMsgType_isBlock(ovlp->msg->type)) {
				ovlp->msg->size = ovlp->pos;
				return ovlp->pos;
			}
		} else {
			if (errno == EINTR)
				goto _retry;

			if (errno != EAGAIN) {
				TRACE2("tcp(%d): %s(%d-%d), size(%d), pos(%d), result = %d, errno = %d.\n",
					tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
					ovlp->perform_count, ovlp->signal_count,
					ovlp->msg->size, ovlp->pos, result, errno);
				return -EIO;
			}
			//ovlp->perform_count += 1;
		}
	} else if (ovlp->perform_count != ovlp->signal_count) {
		TRACE2("tcp(%d): skip %s(%d-%d), size(%d), pos(%d), errno = %d.\n",
			tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
			ovlp->perform_count, ovlp->signal_count,
			ovlp->msg->size, ovlp->pos, errno);
	}

	result = InterlockedCompareExchange(&ovlp->status, op_pending, op_none);
	if (result == op_none)
		return 0;
	if (result != op_signal)
		return -EIO;

	TRACE2("tcp(%d): %s(%d-%d), size(%d), pos(%d), retry again, errno = %d.\n",
		tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
		ovlp->perform_count, ovlp->signal_count,
		ovlp->msg->size, ovlp->pos, errno);
	goto _retry;
}

static void AsyncOvlpSignal(AsyncTcp *tcp, AsyncOvlp *ovlp)
{
	ovlp->signal_count += 1;
	int result = InterlockedExchange(&ovlp->status, op_signal);

	assert(result != op_error);
	if (result == op_pending) {
		if (ovlp->signal_count != ovlp->perform_count+1) {
			TRACE2("tcp(%d): reset %s, perform_count(%d) to signal_count(%d).\n",
				tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
				ovlp->perform_count, ovlp->signal_count);
			ovlp->perform_count = ovlp->signal_count - 1;
		}

		result = AsyncOvlpProc(tcp, ovlp);
		if (result != 0)
			result = ovlp->msg->done(ovlp->msg, result);
	}
}

static void AsyncTcpCloseDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = container_of(asop, AsyncOvlp, sysio);
	AsyncTcp *tcp = ovlp->tcp;
	assert(ovlp == &tcp->recv_ovlp);

	int unbind = AThreadUnbind(&tcp->send_ovlp.sysio);
	if (unbind >= 0)
		AObjectRelease(&tcp->object);

	TRACE2("tcp(%d): close done, unbind = %d, send(%d-%d), recv(%d-%d).\n",
		tcp->sock, unbind,
		tcp->send_ovlp.perform_count, tcp->send_ovlp.signal_count,
		tcp->recv_ovlp.perform_count, tcp->recv_ovlp.signal_count);

	release_s(tcp->sock, closesocket, INVALID_SOCKET);
	result = tcp->recv_ovlp.msg->done(tcp->recv_ovlp.msg, 1);
}

static void AsyncOvlpError(AsyncTcp *tcp, int events)
{
	int unbind = AThreadUnbind(&tcp->send_ovlp.sysio);

	TRACE2("tcp(%d): epoll event = %d, unbind = %d, send(%d-%d), recv(%d-%d).\n",
		tcp->sock, events, unbind,
		tcp->send_ovlp.perform_count, tcp->send_ovlp.signal_count,
		tcp->recv_ovlp.perform_count, tcp->recv_ovlp.signal_count);

	int result = InterlockedExchange(&tcp->recv_ovlp.status, op_error);
	if (result == op_pending)
		result = tcp->recv_ovlp.msg->done(tcp->recv_ovlp.msg, -EIO);

	result = InterlockedExchange(&tcp->send_ovlp.status, op_error);
	if (result == op_pending)
		result = tcp->send_ovlp.msg->done(tcp->send_ovlp.msg, -EIO);

	if (unbind >= 0)
		AObjectRelease(&tcp->object);
	else
		assert(0);
}

static void AsyncTcpRequestDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = container_of(asop, AsyncOvlp, sysio);
	AsyncTcp *tcp = ovlp->tcp;
	assert(ovlp == &tcp->send_ovlp);

	if (result & (EPOLLHUP|EPOLLERR)) {
		AsyncOvlpError(tcp, result);
		return;
	}

	if (result & EPOLLOUT)
		AsyncOvlpSignal(tcp, &tcp->send_ovlp);

	if (result & EPOLLIN)
		AsyncOvlpSignal(tcp, &tcp->recv_ovlp);
}

static void AsyncTcpOpenDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = container_of(asop, AsyncOvlp, sysio);
	AsyncTcp *tcp = ovlp->tcp;
	assert(ovlp == &tcp->send_ovlp);

	if (result & (EPOLLHUP|EPOLLERR)) {
		AsyncOvlpError(tcp, result);
		return;
	}
	
	if (result & (EPOLLIN|EPOLLOUT)) { // checking connect
		tcp->send_ovlp.signal_count += ((result & EPOLLOUT) == EPOLLOUT);
		tcp->recv_ovlp.signal_count += ((result & EPOLLIN) == EPOLLIN);
		TRACE2("tcp(%d): open events = %d, send(%d-%d), recv(%d-%d).\n",
			tcp->sock, result, 
			tcp->send_ovlp.perform_count, tcp->send_ovlp.signal_count,
			tcp->recv_ovlp.perform_count, tcp->recv_ovlp.signal_count);

		int error = 0;
		socklen_t errorlen = sizeof(error);

		result = getsockopt(tcp->sock, SOL_SOCKET, SO_ERROR, (char*)&error, &errorlen);
		if ((result != 0) || (error != 0))
			error = -EIO;
		else
			error = 1;

		result = InterlockedCompareExchange(&ovlp->status, op_none, op_pending);
		if (result == op_pending) {
			ovlp->sysio.callback = &AsyncTcpRequestDone;
			result = ovlp->msg->done(ovlp->msg, error);
		}
	}
}
#endif

static int AsyncTcpBind(AsyncTcp *tcp, AMessage *msg)
{
	tcp->send_ovlp.msg = msg;
	if (msg != NULL)
		tcp->send_ovlp.sysio.callback = &AsyncTcpOpenDone;
	else
		tcp->send_ovlp.sysio.callback = &AsyncTcpRequestDone;

#ifdef _WIN32
	tcp->recv_ovlp.sysio.callback = &AsyncTcpRequestDone;
	int result = AThreadBind(NULL, (HANDLE)tcp->sock);
#else
	tcp->send_ovlp.status = (msg ? op_pending : op_none);
	tcp->send_ovlp.sysio.ao_fd = tcp->sock;
	tcp->send_ovlp.perform_count = 0;
	tcp->send_ovlp.signal_count = 0;

	tcp->recv_ovlp.status = op_none;
	tcp->recv_ovlp.sysio.ao_fd = tcp->sock;
	tcp->recv_ovlp.sysio.callback = &AsyncTcpCloseDone;
	tcp->recv_ovlp.perform_count = 0;
	tcp->recv_ovlp.signal_count = 0;

	AObjectAddRef(&tcp->object);
	int result = AThreadBind(NULL, &tcp->send_ovlp.sysio, EPOLLIN|EPOLLOUT|EPOLLET|EPOLLHUP|EPOLLERR);
	if (result < 0) {
		AObjectRelease(&tcp->object);
		result = -EIO;
	}
#endif
	return (result < 0 ? result : 1);
}

static int AsyncTcpOpen(AObject *object, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);
	if (tcp->sock != INVALID_SOCKET)
		return -EBUSY;

	if (msg->type == AMsgType_Handle) {
		if (msg->size != 0)
			return -EINVAL;

		tcp->sock = (SOCKET)msg->data;
		if (tcp->sock == INVALID_SOCKET)
			return 1;
#ifndef _WIN32
		if (tcp_nonblock(tcp->sock, 1) != 0)
			return -EIO;
#endif
		return AsyncTcpBind(tcp, NULL);
	}

	if ((msg->type != AMsgType_Option)
	 || (msg->data == NULL)
	 || (msg->size != 0))
		return -EINVAL;

	AOption *option = (AOption*)msg->data;
	AOption *addr = AOptionFind(option, "address");
	if (addr == NULL)
		return -EINVAL;

	AOption *port = AOptionFind(option, "port");
	//if (port == NULL)
	//	return -EINVAL;

	struct addrinfo *ai = tcp_getaddrinfo(addr->value, port?port->value:NULL);
	if (ai == NULL) {
		TRACE("path(%s:%s) error = %d.\n", addr->value, port?port->value:"", errno);
		return -EFAULT;
	}

	tcp->sock = socket(ai->ai_family, SOCK_STREAM, ai->ai_protocol);
	if (tcp->sock == INVALID_SOCKET) {
		release_s(ai, freeaddrinfo, NULL);
		return -EFAULT;
	}

#ifdef _WIN32
	int result = AsyncTcpBind(tcp, msg);
	if (result >= 0)
		result = iocp_connect(tcp->sock, ai->ai_addr, ai->ai_addrlen, &tcp->send_ovlp.sysio.ao_ovlp);
	if (result > 0)
		result = 0;
#else
	int result = tcp_nonblock(tcp->sock, 1);
	if (result == 0)
		result = connect(tcp->sock, ai->ai_addr, ai->ai_addrlen);

	if ((result != 0) && (errno != EINPROGRESS)) {
		result = -EIO;
	} else {
		result = AsyncTcpBind(tcp, msg);
		if (result >= 0)
			result = 0;
	}
#endif
	release_s(ai, freeaddrinfo, NULL);
	return result;
}

static int AsyncTcpRequest(AObject *object, int reqix, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);

	assert(msg->size != 0);
	switch (reqix)
	{
	case Aio_Input:
		tcp->send_ovlp.msg = msg;
#ifdef _WIN32
		tcp->send_ovlp.buf.buf = msg->data;
		tcp->send_ovlp.buf.len = msg->size;
		return iocp_sendv(tcp->sock, &tcp->send_ovlp.buf, 1, &tcp->send_ovlp.sysio.ao_ovlp);
#else
		tcp->send_ovlp.pos = 0;
		return AsyncOvlpProc(tcp, &tcp->send_ovlp);
#endif
	case Aio_Output:
		tcp->recv_ovlp.msg = msg;
#ifdef _WIN32
		tcp->recv_ovlp.buf.buf = msg->data;
		tcp->recv_ovlp.buf.len = msg->size;
		return iocp_recvv(tcp->sock, &tcp->recv_ovlp.buf, 1, &tcp->recv_ovlp.sysio.ao_ovlp);
#else
		tcp->recv_ovlp.pos = 0;
		return AsyncOvlpProc(tcp, &tcp->recv_ovlp);
#endif
	default:
		assert(FALSE);
		return -ENOSYS;
	}
}

static int AsyncTcpCancel(AObject *object, int reqix, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (reqix == Aio_Input) {
		shutdown(tcp->sock, SD_SEND);
	} else if (reqix == Aio_Output) {
		shutdown(tcp->sock, SD_RECEIVE);
	} else {
		assert(FALSE);
		return -ENOSYS;
	}
	return 1;
}

static int AsyncTcpClose(AObject *object, AMessage *msg)
{
	AsyncTcp *tcp = to_tcp(object);
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (msg == NULL) {
		shutdown(tcp->sock, SD_BOTH);
		return 1;
	}
#ifdef _WIN32
	release_s(tcp->sock, closesocket, INVALID_SOCKET);
	return 1;
#else
	assert(tcp->send_ovlp.status != op_pending);
	assert(tcp->recv_ovlp.status != op_pending);

	if (tcp->send_ovlp.sysio.ao_events == 0) {
		release_s(tcp->sock, closesocket, INVALID_SOCKET);
		return 1;
	}

	tcp->recv_ovlp.msg = msg;
	tcp->recv_ovlp.sysio.callback = &AsyncTcpCloseDone;
	tcp->recv_ovlp.sysio.ao_thread = tcp->send_ovlp.sysio.ao_thread;

	int result = AThreadPost(tcp->send_ovlp.sysio.ao_thread, &tcp->recv_ovlp.sysio, TRUE);
	return (result < 0 ? result : 0);
#endif
}

static int AsyncTcpInit(AOption *option)
{
#ifdef _WIN32
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);
#endif
	return 1;
}

AModule AsyncTcpModule = {
	"io",
	"async_tcp",
	sizeof(AsyncTcp),
	AsyncTcpInit, NULL,
	&AsyncTcpCreate,
	&AsyncTcpRelease,
	NULL,
	2,

	&AsyncTcpOpen,
	NULL,
	NULL,
	&AsyncTcpRequest,
	&AsyncTcpCancel,
	&AsyncTcpClose,
};
