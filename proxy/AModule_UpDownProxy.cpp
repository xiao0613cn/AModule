#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"

static AObject *up_io = NULL;
static AOption *up_option = NULL;
static ARefsMsg up_msg = { 0 };
static AOperator up_timer;

static AObject *down_io = NULL;
static AOption *down_option = NULL;
static AMessage down_msg = { 0 };
static AOperator down_timer;
static char     down_buf[2048];
static BOOL     down_output = FALSE;

enum status {
	post_none = 0,
	up_opening,
	up_opened,
	down_opening,

	up_output,
	down_input,

	post_closing,
	up_closing,
	down_closing,
} post_status;

static int PostClose(AMessage *msg, int result)
{
	assert(msg == &up_msg.msg);
	switch (post_status)
	{
	case post_closing:
		AMsgInit(&up_msg.msg, AMsgType_Unknown, NULL, 0);
		post_status = up_closing;
		result = up_io->close(&up_msg.msg);
		if (result == 0)
			return 0;

	case up_closing:
		AMsgInit(&up_msg.msg, AMsgType_Unknown, NULL, 0);
		post_status = down_closing;
		result = down_io->close(&up_msg.msg);
		if (result == 0)
			return 0;

	case down_closing:
		post_status = post_none;
		AOperatorTimewait(&up_timer, NULL, 10*1000);
		return result;

	default:
		assert(FALSE);
		return -EACCES;
	}
}

static int DownMsg(AMessage *msg, int result)
{
	assert(msg == &down_msg);
	while (result > 0) {
		if (post_status == post_closing) {
			result = -EINTR;
			break;
		}

		if (down_output) {
			down_msg.type = ioMsgType_Block;
			down_output = FALSE;
			result = ioInput(up_io, &down_msg);
		} else {
			down_output = TRUE;
			result = ioOutput(down_io, &down_msg, down_buf, sizeof(down_buf));
		}
	}
	if (result < 0) {
		down_msg.data = NULL; // MARK down_io not output
	}
	return result;
}

static int DownTimer(AOperator *asop, int result)
{
	assert(asop == &down_timer);
	down_output = FALSE;
	down_msg.done = &DownMsg;
	return DownMsg(&down_msg, 1);
}

static int UpMsg(AMessage *msg, int result)
{
	assert(msg == &up_msg.msg);
	while (result > 0) {
		switch (post_status)
		{
		case post_none:
			AMsgInit(&up_msg.msg, AMsgType_Option, up_option, 0);
			post_status = up_opening;
			result = up_io->open(&up_msg.msg);
			break;

		case up_opening:
			AMsgInit(&up_msg.msg, AMsgType_Unknown, NULL, 0);
			post_status = up_opened;
			result = ioInput(up_io, &up_msg.msg);
			break;

		case up_opened:
			AMsgInit(&up_msg.msg, AMsgType_Option, down_option, 0);
			post_status = down_opening;
			result = down_io->open(&up_msg.msg);
			break;

		case down_opening:
			down_msg.data = down_buf; // MARK down_io output
			down_timer.done = &DownTimer;
			AOperatorTimewait(&down_timer, NULL, 0);

		case down_input:
			AMsgInit(&up_msg.msg, AMsgType_RefsMsg, &up_msg, 0);
			post_status = up_output;
			result = ioOutput(up_io, &up_msg.msg);
			break;

		case up_output:
			if (up_msg.msg.type == AMsgType_RefsMsg)
				AMsgInit(&up_msg.msg, ioMsgType_Block, up_msg.ptr(), up_msg.size);
			else
				up_msg.msg.type = ioMsgType_Block;
			post_status = down_input;
			result = ioInput(down_io, &up_msg.msg);
			break;

		default:
			assert(FALSE);
			break;
		}
	}
	if (result < 0) {
		post_status = post_closing;
		up_timer.done(&up_timer, 1);
	}
	return result;
}

static int UpTimer(AOperator *asop, int result)
{
	assert(asop == &up_timer);
	if (result < 0)
		return result;

	if (post_status == post_closing) {
		if (down_msg.data != NULL) { //TEST down_io is output
			AOperatorTimewait(&up_timer, NULL, 1000);
		} else {
			up_msg.msg.done = &PostClose;
			PostClose(&up_msg.msg, result);
		}
	} else {
		assert(post_status == post_none);
		up_msg.msg.done = &UpMsg;
		UpMsg(&up_msg.msg, 1);
	}
	return result;
}

static int PostInit(AOption *global_option, AOption *module_option, int first)
{
	if (module_option == NULL)
		return 0;

	up_option = AOptionFind(module_option, "up_io");
	down_option = AOptionFind(module_option, "down_io");

	int result = AObjectCreate(&up_io, NULL, up_option, NULL);
	if (result >= 0)
		result = AObjectCreate(&down_io, NULL, down_option, NULL);
	if (result < 0) {
		release_s(up_io, AObjectRelease, NULL);
		release_s(down_io, AObjectRelease, NULL);
		return 0;
	}

	post_status = post_none;
	up_timer.done = &UpTimer;
	AOperatorTimewait(&up_timer, NULL, 10*1000);
	return 1;
}

AModule UpDownProxyModule = {
	"io_proxy",
	"up_down_proxy",
	0,
	&PostInit, NULL,
};
