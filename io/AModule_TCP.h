#ifndef _AMODULE_TCP_H_
#define _AMODULE_TCP_H_


struct TCPObject {
	AObject object;
	SOCKET  sock;

	char    recvbuf[BUFSIZ];
	long    recvsiz;
	long    peekpos;
};
#define to_tcp(obj) CONTAINING_RECORD(obj, TCPObject, object);

extern AModule TCPModule;

#endif
