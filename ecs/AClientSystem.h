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
	int     _tick_reopen;  // 30*1000
	int     _tick_heart;   // 10*1000
	int     _tick_abort;   // 20*1000
	bool    _owner_thread; // false
	bool    _open_heart;   // true
	bool    _auto_reopen;  // true

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

	int (*open)(AClientComponent *c);
	int (*heart)(AClientComponent *c);
	int (*abort)(AClientComponent *c);
	int (*close)(AClientComponent *c);
};





#endif
