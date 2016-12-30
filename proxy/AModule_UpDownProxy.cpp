#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../io/AModule_io.h"

static AObject *up_io = NULL;
static AOption *up_option = NULL;

static AObject *down_io = NULL;
static AOption *down_option = NULL;

static AMessage down_msg = { 0 };
static char     down_buf[BUFSIZ];
static AOperator down_timer;
static BOOL     down_output = FALSE;

static ARefsMsg post_msg;
static AOperator post_timer;

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
	assert(msg == &post_msg.msg);
	switch (post_status)
	{
	case post_closing:
		AMsgInit(&post_msg.msg, AMsgType_Unknown, NULL, 0);
		post_status = up_closing;
		result = up_io->close(up_io, &post_msg.msg);
		if (result == 0)
			return 0;

	case up_closing:
		AMsgInit(&post_msg.msg, AMsgType_Unknown, NULL, 0);
		post_status = down_closing;
		result = down_io->close(down_io, &post_msg.msg);
		if (result == 0)
			return 0;

	case down_closing:
		post_status = post_none;
		AOperatorTimewait(&post_timer, NULL, 10*1000);
		return result;

	default:
		assert(FALSE);
		return -EACCES;
	}
}

static int DownMsg(AMessage *msg, int result)
{
	assert(msg == &down_msg);
	do {
		if (down_output) {
			down_output = FALSE;
			result = ioInput(up_io, &down_msg);
		} else {
			AMsgInit(&down_msg, AMsgType_Unknown, down_buf, sizeof(down_buf));
			down_output = TRUE;
			result = ioOutput(down_io, &down_msg);
		}
	} while (result > 0);
	if (result < 0) {
		down_msg.data = NULL; // MARK down_io not output
	}
	return result;
}

static void DownTimer(AOperator *asop, int result)
{
	assert(asop == &down_timer);
	down_output = FALSE;
	down_msg.done = &DownMsg;
	DownMsg(&down_msg, 1);
}

static int PostMsg(AMessage *msg, int result)
{
	assert(msg == &post_msg.msg);
	while (result > 0) {
		switch (post_status)
		{
		case post_none:
			AMsgInit(&post_msg.msg, AMsgType_Option, up_option, 0);
			post_status = up_opening;
			result = up_io->open(up_io, &post_msg.msg);
			break;

		case up_opening:
			AMsgInit(&post_msg.msg, AMsgType_Unknown, NULL, 0);
			post_status = up_opened;
			result = ioInput(up_io, &post_msg.msg);
			break;

		case up_opened:
			AMsgInit(&post_msg.msg, AMsgType_Option, down_option, 0);
			post_status = down_opening;
			result = down_io->open(down_io, &post_msg.msg);
			break;

		case down_opening:
			down_msg.data = down_buf; // MARK down_io output
			down_timer.callback = &DownTimer;
			AOperatorTimewait(&down_timer, NULL, 0);

		case down_input:
			AMsgInit(&post_msg.msg, AMsgType_RefsMsg, &post_msg, 0);
			post_status = up_output;
			result = ioOutput(up_io, &post_msg.msg);
			break;

		case up_output:
			AMsgInit(&post_msg.msg, AMsgType_Unknown, post_msg.ptr(), post_msg.size);
			post_status = down_input;
			result = ioInput(down_io, &post_msg.msg);
			break;

		default:
			assert(FALSE);
			break;
		}
	}
	if (result < 0) {
		post_status = post_closing;
		AOperatorTimewait(&post_timer, NULL, 0);
	}
	return result;
}

static void PostTimer(AOperator *asop, int result)
{
	assert(asop == &post_timer);
	if (result < 0)
		return;

	if (post_status == post_closing) {
		if (down_msg.data != NULL) { //TEST down_io is output
			AOperatorTimewait(&post_timer, NULL, 1000);
		} else {
			post_msg.msg.done = &PostClose;
			PostClose(&post_msg.msg, result);
		}
	} else {
		assert(post_status == post_none);
		post_msg.msg.done = &PostMsg;
		PostMsg(&post_msg.msg, 1);
	}
}

static int PostInit(AOption *global_option, AOption *module_option)
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
	post_timer.callback = &PostTimer;
	AOperatorTimewait(&post_timer, NULL, 10*1000);
	return 1;
}

AModule UpDownProxyModule = {
	"io_proxy",
	"up_down_pair",
	0,
	&PostInit, NULL,
};
