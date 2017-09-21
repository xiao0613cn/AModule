#ifndef _ACLIENT_COMPONENT_H_
#define _ACLIENT_COMPONENT_H_

#include "ASystem.h"

typedef struct AClientComponent AClientComponent;
typedef struct AClientSystem AClientSystem;

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

	AClientSystem *_system;
	AOperator      _system_asop;
	//rb_node        _system_node;

	int (*open)(AClientComponent *c);
	int (*heart)(AClientComponent *c);
	int (*abort)(AClientComponent *c);
	int (*close)(AClientComponent *c);
};

rb_tree_declare(AClientComponent, AClientComponent*);

struct AClientSystem : public ASystem {
	//rb_root   _client_map;
	//int       _client_count;

	//DWORD     _exec_tick;
	//AOperator _exec_asop;
	//bool      _exec_abort;
	//AClientComponent *_exec_last;

	//void  init() {
		//exec_check = &ExecCheck<AClientSystem, AClientComponent, -1>;
		//exec_one = &ExecOne;
		//exec_post = &ExecPost;
	//}
	//bool  _push(AClientComponent *c);
	//bool  _pop(AClientComponent *c);

	enum Result _exec_check(AClientComponent *c, DWORD cur_tick);
	int   _exec_one(AClientComponent *c, int result);
};










#endif
