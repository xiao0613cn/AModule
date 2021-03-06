#ifndef _ACLIENT_COMPONENT_H_
#define _ACLIENT_COMPONENT_H_

#include "AEntity.h"
#include "ASystem.h"


struct AClientComponent : public AComponent {
	static const char* name() { return "AClientComponent"; }
	AMODULE_GET(AModule, "ASystem", name())

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

	// options, get | set
	int     _tick_reopen;
	int     _tick_heart;
	int     _tick_abort;
	bool    _owner_thread;
	bool    _open_heart;
	bool    _auto_reopen;
	bool    _auto_remove; // if (!_auto_reopen && _auto_remove) em->_pop(_entity);

	// data
	enum Status   _status;     // get only
	DWORD         _main_tick;
	bool volatile _main_abort;
	bool valid() { return ((_status == Opened) && !_main_abort); }

	bool          _last_opened;
	enum Heart    _check_heart; // get only
	long volatile _busy_count;
	long use(int count) { return InterlockedAdd(&_busy_count, count); }

	list_head       _sys_node;     // private
	ASystem::Result _run_result;   // private
	ASystem::Result _abort_result; // private
	void exec_done(int result) {   // private
		assert(_run_result.status == ASystem::Runnable);
		_run_result.system->exec_run(&_run_result, result);
	}

	int (*open)(AClientComponent *c);
	int (*heart)(AClientComponent *c);
	int (*abort)(AClientComponent *c);
	int (*close)(AClientComponent *c);

	void init2() {
		_tick_reopen  = 30*1000; _tick_heart  = 10*1000;   _tick_abort  = 20*1000;
		_owner_thread = false;   _open_heart  = true;      _auto_reopen = true;    _auto_remove = true;
		_status       = Invalid; _main_tick   = 0;         _main_abort  = false;
		_last_opened  = false;   _check_heart = HeartNone; _busy_count  = 0;
		_sys_node.init(); _run_result.status = _abort_result.status = ASystem::NotNeed;

		// set by implement module
		open = NULL; heart = NULL; abort = NULL; close = NULL;
	}
	void exit2() {
	}
	const char* status_name() {
		switch (_status)
		{
		case Invalid: return "[Invalid]";
		case Opening: return "[Opening]";
		case Opened:  return "[Opened]";
		case Closing: return "[Closing]";
		case Closed:  return "[Closed]";
		default:      return "[Unknown]";
		}
	}
};


#endif
