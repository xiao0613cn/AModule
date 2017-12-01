#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AModule_io.h"

enum op_status {
	op_none = 0,
	op_pending,
	op_signal,
	op_error,
};

struct AsyncOvlp : public AOperator {
	struct AsyncTcp *tcp;
	AMessage *from;
#ifdef _WIN32
	WSABUF    buf;
#else
	int       pos;
	unsigned int  perform_count;
	unsigned int  signal_count;
	long volatile status;
#endif
};

struct AsyncTcp : public IOObject {
	SOCKET    sock;
	AsyncOvlp send_ovlp;
	AsyncOvlp recv_ovlp;
};

static void AsyncTcpRelease(AObject *object)
{
	AsyncTcp *tcp = (AsyncTcp*)object;
	closesocket_s(tcp->sock);
}

static int AsyncTcpCreate(AObject **object, AObject *parent, AOption *option)
{
	AsyncTcp *tcp = (AsyncTcp*)*object;
	tcp->sock = INVALID_SOCKET;

	z_set(tcp->send_ovlp).tcp = tcp;
	z_set(tcp->recv_ovlp).tcp = tcp;
	return 1;
}

#ifdef _WIN32
static int AsyncTcpRequestDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = (AsyncOvlp*)asop;
	if (result <= 0)
		return ovlp->from->done2(-EIO);

	if (!ioMsgType_isBlock(ovlp->from->type)) {
		ovlp->from->size = result;
		return ovlp->from->done2(result);
	}

	ovlp->buf.buf += result;
	ovlp->buf.len -= result;

	if (ovlp->buf.len == 0) {
		result = ovlp->from->size;
	} else {
		AsyncTcp *tcp = ovlp->tcp;
		if (ovlp == &tcp->send_ovlp)
			result = iocp_sendv(tcp->sock, &ovlp->buf, 1, &ovlp->ao_ovlp);
		else
			result = iocp_recvv(tcp->sock, &ovlp->buf, 1, &ovlp->ao_ovlp);
		if (result == 0)
			return 0;
	}
	return ovlp->from->done2(result);
}

static int AsyncTcpOpenDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = (AsyncOvlp*)asop;
	AsyncTcp *tcp = ovlp->tcp;

	if (result >= 0)
		result = iocp_is_connected(tcp->sock);

	tcp->send_ovlp.done = &AsyncTcpRequestDone;
	tcp->recv_ovlp.done = &AsyncTcpRequestDone;
	return ovlp->from->done2(result);
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
			result = send(tcp->sock, ovlp->from->data+ovlp->pos, ovlp->from->size-ovlp->pos, MSG_NOSIGNAL);
		} else {
			result = recv(tcp->sock, ovlp->from->data+ovlp->pos, ovlp->from->size-ovlp->pos, 0);
		}
		if (result == 0)
			return -EIO;

		if (result > 0) {
			ovlp->pos += result;
			if (ovlp->pos == ovlp->from->size)
				return ovlp->pos;

			ovlp->perform_count += 1;
			if (!ioMsgType_isBlock(ovlp->from->type)) {
				ovlp->from->size = ovlp->pos;
				return ovlp->pos;
			}
		} else {
			if (errno == EINTR)
				goto _retry;

			if (errno != EAGAIN) {
				TRACE2("tcp(%d): %s(%d-%d), size(%d), pos(%d), result = %d, errno = %d.\n",
					tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
					ovlp->perform_count, ovlp->signal_count,
					ovlp->from->size, ovlp->pos, result, errno);
				return -EIO;
			}
			//ovlp->perform_count += 1;
		}
	} else if (ovlp->perform_count != ovlp->signal_count) {
		TRACE2("tcp(%d): skip %s(%d-%d), size(%d), pos(%d), errno = %d.\n",
			tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
			ovlp->perform_count, ovlp->signal_count,
			ovlp->from->size, ovlp->pos, errno);
	}

	result = InterlockedCompareExchange(&ovlp->status, op_pending, op_none);
	if (result == op_none)
		return 0;
	if (result != op_signal)
		return -EIO;

	TRACE2("tcp(%d): %s(%d-%d), size(%d), pos(%d), retry again, errno = %d.\n",
		tcp->sock, (ovlp==&tcp->send_ovlp)?"send":"recv",
		ovlp->perform_count, ovlp->signal_count,
		ovlp->from->size, ovlp->pos, errno);
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
			result = ovlp->from->done2(result);
	}
}

static int AsyncTcpCloseDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = (AsyncOvlp*)asop;
	AsyncTcp *tcp = ovlp->tcp;
	assert(ovlp == &tcp->recv_ovlp);

	int unbind = AThreadUnbind(&tcp->send_ovlp);
	if (unbind >= 0)
		tcp->release();

	TRACE2("tcp(%d): close done, unbind = %d, send(%d-%d), recv(%d-%d).\n",
		tcp->sock, unbind,
		tcp->send_ovlp.perform_count, tcp->send_ovlp.signal_count,
		tcp->recv_ovlp.perform_count, tcp->recv_ovlp.signal_count);

	closesocket_s(tcp->sock);
	return ovlp->from->done2(1);
}

static void AsyncOvlpError(AsyncOvlp *ovlp, AsyncTcp *tcp, int events)
{
	int unbind = AThreadUnbind(ovlp);

	TRACE2("tcp(%d): epoll event = %d, unbind = %d, send(%d-%d), recv(%d-%d).\n",
		tcp->sock, events, unbind,
		tcp->send_ovlp.perform_count, tcp->send_ovlp.signal_count,
		tcp->recv_ovlp.perform_count, tcp->recv_ovlp.signal_count);

	int result = InterlockedExchange(&tcp->recv_ovlp.status, op_error);
	if (result == op_pending)
		result = tcp->recv_ovlp.from->done2(-EIO);

	result = InterlockedExchange(&tcp->send_ovlp.status, op_error);
	if (result == op_pending)
		result = tcp->send_ovlp.from->done2(-EIO);

	if (unbind >= 0)
		tcp->release();
	else
		assert(0);
}

static int AsyncTcpRequestDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = (AsyncOvlp*)asop;
	AsyncTcp *tcp = ovlp->tcp;
	assert(ovlp == &tcp->send_ovlp);

	if ((result < 0) || (result & (EPOLLHUP|EPOLLERR))) {
		AsyncOvlpError(ovlp, tcp, result);
		return result;
	}

	if (result & EPOLLOUT)
		AsyncOvlpSignal(tcp, &tcp->send_ovlp);

	if (result & EPOLLIN)
		AsyncOvlpSignal(tcp, &tcp->recv_ovlp);
	return result;
}

static int AsyncTcpOpenDone(AOperator *asop, int result)
{
	AsyncOvlp *ovlp = (AsyncOvlp*)asop;
	AsyncTcp *tcp = ovlp->tcp;
	assert(ovlp == &tcp->send_ovlp);

	if ((result < 0) || (result & (EPOLLHUP|EPOLLERR))) {
		AsyncOvlpError(ovlp, tcp, result);
		return result;
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
			ovlp->done = &AsyncTcpRequestDone;
			result = ovlp->from->done2(error);
		}
	}
	return result;
}
#endif

static int AsyncTcpBind(AsyncTcp *tcp, AMessage *msg)
{
	tcp->send_ovlp.from = msg;
	if (msg != NULL)
		tcp->send_ovlp.done = &AsyncTcpOpenDone;
	else
		tcp->send_ovlp.done = &AsyncTcpRequestDone;

#ifdef _WIN32
	tcp->recv_ovlp.done = &AsyncTcpRequestDone;
	int result = AThreadBind(NULL, (HANDLE)tcp->sock);
#else
	tcp->send_ovlp.status = (msg ? op_pending : op_none);
	tcp->send_ovlp.ao_fd = tcp->sock;
	tcp->send_ovlp.perform_count = 0;
	tcp->send_ovlp.signal_count = 0;

	tcp->recv_ovlp.status = op_none;
	tcp->recv_ovlp.ao_fd = tcp->sock;
	tcp->recv_ovlp.done = &AsyncTcpCloseDone;
	tcp->recv_ovlp.perform_count = 0;
	tcp->recv_ovlp.signal_count = 0;

	tcp->addref();
	int result = AThreadBind(NULL, &tcp->send_ovlp, EPOLLIN|EPOLLOUT|EPOLLET|EPOLLHUP|EPOLLERR);
	if (result < 0) {
		tcp->release();
		result = -EIO;
	}
#endif
	return (result < 0 ? result : 1);
}

static int AsyncTcpOpen(AObject *object, AMessage *msg)
{
	AsyncTcp *tcp = (AsyncTcp*)object;
	if (tcp->sock != INVALID_SOCKET)
		return -EBUSY;

	if (msg->type == AMsgType_Handle) {
		if (msg->size != 0)
			return -EINVAL;

		tcp->sock = (SOCKET)(long)msg->data;
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
	const char *addr = option->getStr("address", NULL);
	if (addr == NULL)
		return -EINVAL;

	const char *port = option->getStr("port", NULL);
	//if (port == NULL)
	//	return -EINVAL;

	struct addrinfo *ai = net_getaddrinfo(addr, port);
	if (ai == NULL) {
		TRACE("path(%s:%s) error = %d.\n", addr, port, errno);
		return -EFAULT;
	}
	godefer(addrinfo*, ai, freeaddrinfo(ai));

	struct addrinfo *ai_valid = ai;
	do {
		if (ai_valid->ai_protocol != IPPROTO_UDP)
			break;
	} while ((ai_valid = ai_valid->ai_next) != NULL);
	if (ai_valid == NULL) {
		TRACE("invalid address: %s:%s, ai_protocol = %d.\n",
			addr, port, ai->ai_protocol);
		return -EINVAL;
	}

	tcp->sock = socket(ai_valid->ai_family, SOCK_STREAM, ai_valid->ai_protocol);
	if (tcp->sock == INVALID_SOCKET) {
		return -EFAULT;
	}

#ifdef _WIN32
	int result = AsyncTcpBind(tcp, msg);
	if (result >= 0)
		result = iocp_connect(tcp->sock, ai_valid->ai_addr, ai_valid->ai_addrlen, &tcp->send_ovlp.ao_ovlp);
	if (result > 0)
		result = 0;
#else
	int result = tcp_nonblock(tcp->sock, 1);
	if (result == 0)
		result = connect(tcp->sock, ai_valid->ai_addr, ai_valid->ai_addrlen);

	if ((result != 0) && (errno != EINPROGRESS)) {
		result = -EIO;
	} else {
		result = AsyncTcpBind(tcp, msg);
		if (result >= 0)
			result = 0;
	}
#endif
	return result;
}

static int AsyncTcpRequest(AObject *object, int reqix, AMessage *msg)
{
	AsyncTcp *tcp = (AsyncTcp*)object;

	assert(msg->size != 0);
	switch (reqix)
	{
	case Aio_Input:
		tcp->send_ovlp.from = msg;
#ifdef _WIN32
		tcp->send_ovlp.buf.buf = msg->data;
		tcp->send_ovlp.buf.len = msg->size;
		return iocp_sendv(tcp->sock, &tcp->send_ovlp.buf, 1, &tcp->send_ovlp.ao_ovlp);
#else
		tcp->send_ovlp.pos = 0;
		return AsyncOvlpProc(tcp, &tcp->send_ovlp);
#endif
	case Aio_Output:
		tcp->recv_ovlp.from = msg;
#ifdef _WIN32
		tcp->recv_ovlp.buf.buf = msg->data;
		tcp->recv_ovlp.buf.len = msg->size;
		return iocp_recvv(tcp->sock, &tcp->recv_ovlp.buf, 1, &tcp->recv_ovlp.ao_ovlp);
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
	AsyncTcp *tcp = (AsyncTcp*)object;
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
	AsyncTcp *tcp = (AsyncTcp*)object;
	if (tcp->sock == INVALID_SOCKET)
		return -ENOENT;

	if (msg == NULL) {
		shutdown(tcp->sock, SD_BOTH);
		return 1;
	}
#ifdef _WIN32
	closesocket_s(tcp->sock);
	return 1;
#else
	assert(tcp->send_ovlp.status != op_pending);
	assert(tcp->recv_ovlp.status != op_pending);

	if (tcp->send_ovlp.ao_events == 0) {
		closesocket_s(tcp->sock);
		return 1;
	}

	tcp->recv_ovlp.from = msg;
	tcp->recv_ovlp.done = &AsyncTcpCloseDone;
	tcp->recv_ovlp.ao_thread = tcp->send_ovlp.ao_thread;

	int result = tcp->recv_ovlp.post(tcp->send_ovlp.ao_thread);
	return (result < 0 ? result : 0);
#endif
}

static int AsyncTcpInit(AOption *global_option, AOption *module_option, BOOL first)
{
#ifdef _WIN32
	if (first) {
		WSADATA wsadata;
		WSAStartup(WINSOCK_VERSION, &wsadata);
	}
#endif
	return 1;
}

static int AsyncTcpAccept(AObject *object, AMessage *msg, AObject *svc_data, AOption *svc_opt)
{
	AsyncTcp *tcp = (AsyncTcp*)object;
	if (msg->type != AMsgType_Handle)
		return -EINVAL;

	closesocket_s(tcp->sock);
	tcp->sock = (SOCKET)msg->data;
#ifndef _WIN32
	if (tcp_nonblock(tcp->sock, 1) != 0)
		return -EIO;
#endif
	return AsyncTcpBind(tcp, NULL);
}


IOModule AsyncTcpModule = { {
	"io",
	"async_tcp",
	sizeof(AsyncTcp),
	&AsyncTcpInit, NULL,
	&AsyncTcpCreate,
	&AsyncTcpRelease, },
	&AsyncTcpOpen,
	&IOModule::OptNull,
	&IOModule::OptNull,
	&AsyncTcpRequest,
	&AsyncTcpCancel,
	&AsyncTcpClose,

	NULL, &AsyncTcpAccept,
};

static int reg_code = AModuleRegister(&AsyncTcpModule.module);
