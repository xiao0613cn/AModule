#ifndef _ACLIENT_COMPONENT_H_
#define _ACLIENT_COMPONENT_H_

#include "../base/ASystem.h"

typedef struct AClientComponent AClientComponent;
typedef struct AClientSystem AClientSystem;

struct AClientComponent : public AComponent {
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
	bool    _auto_reopen;  // true
	bool    _open_heart;   // true

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

	void init(AObject *o, int i = 0) {
		AComponent::init(o, "AClientComponent", i);
	}
};

rb_tree_declare(AClientComponent, AClientComponent*);

struct AClientSystem : public ASystem {
	//rb_root   _client_map;
	//int       _client_count;

	//DWORD     _exec_tick;
	//AOperator _exec_asop;
	//bool      _exec_abort;
	//AClientComponent *_exec_last;

	void  init() {
		exec_check = &ExecCheck;
		exec_one = &ExecOne;
		exec_post = &ExecPost;
	}
	//bool  _push(AClientComponent *c);
	//bool  _pop(AClientComponent *c);

	enum Result _exec_check(AClientComponent *c, DWORD cur_tick);
	int   _exec_one(AClientComponent *c, int result);

	static enum Result ExecCheck(ASystem *s, AEntity *e, DWORD cur_tick) {
		AClientSystem *cs = (AClientSystem*)s;
		AClientComponent *cc = (AClientComponent*)e->_get("AClientComponent");
		if (cc == NULL)
			return Invalid;
		return cs->_exec_check(cc, cur_tick);
	};
	static int ExecOne(ASystem *s, AEntity *e, int result) {
		AClientSystem *cs = (AClientSystem*)s;
		AClientComponent *cc = (AClientComponent*)e->_get("AClientComponent");
		assert(cc != NULL);
		cs->_exec_one(cc, result);
	}
	static void ExecPost(ASystem *s, AEntity *e, bool addref) {
		AClientSystem *cs = (AClientSystem*)s;
		AClientComponent *cc = (AClientComponent*)e->_get("AClientComponent");
		assert(cc != NULL);
		cc->_system_asop.post(cs->_exec_thread);
	}
};










#endif
