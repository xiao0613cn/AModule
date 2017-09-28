#ifndef _ACLIENT_COMPONENT_H_
#define _ACLIENT_COMPONENT_H_

#include "ASystem.h"


struct AClientComponent : public AComponent {
	static const char* name() { return "AClientComponent"; }

	enum Status {
		Invalid = 0,
		Opening,
		Opened,
		Closing,
		Closed,
	};
	enum Heart {
		HeartNone = 0,
		HeartChecking,
		HeartCheckDone,
	};

	// option
	int     _tick_reopen;
	int     _tick_heart;
	int     _tick_abort;
	bool    _owner_thread;
	bool    _open_heart;
	bool    _auto_reopen;

	// data
	enum Status   _status;
	DWORD         _main_tick;
	bool volatile _main_abort;
	bool  valid() { return ((_status == Opened) && !_main_abort); }

	bool          _last_opened;
	enum Heart    _check_heart;
	long volatile _busy_count;
	long  use(int count) { return InterlockedAdd(&_busy_count, count); }

	list_head       _sys_node;
	ASystem::Result _run_result;
	ASystem::Result _abort_result;
	void exec_done(int result) { _run_result.system->exec_run(&_run_result, result); }

	int (*open)(AClientComponent *c);
	int (*heart)(AClientComponent *c);
	int (*abort)(AClientComponent *c);
	int (*close)(AClientComponent *c);

	void init(AObject *o) {
		AComponent::init(o, name());
		_tick_reopen = 30*1000; _tick_heart = 10*1000; _tick_abort = 20*1000;
		_owner_thread = false;  _open_heart = true;    _auto_reopen = true;
		_status = Invalid;      _main_tick = 0;        _main_abort = false;
		_last_opened = false; _check_heart = HeartNone; _busy_count = 0;
		_sys_node.init();
		//open, heart, abort, close
	}
};





#endif
