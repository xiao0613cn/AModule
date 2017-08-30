#ifndef _AMODULE_ACLIENT_H_
#define _AMODULE_ACLIENT_H_


struct AClient : public AObject
{
	AObject_Status status;
	struct rb_node cm_node; // AClientManager::client_map
	struct AClientManager *manager;
	void     (*on_open)(AClient*);
	void     (*on_close)(AClient*);
	void     (*on_erase)(AClient*);

	AOperator  main_asop;
	AMessage   main_msg;
	int        main_run(int result);
	inline void main_end(AObject_Status s) {
		main_msg.done = NULL;
		if (s != AObject_Invalid)
			status = s;
		main_asop.callback = NULL;
		release2();
	}

	DWORD      active;
	BOOL       last_opened  : 1;
	BOOL       owner_thread : 1;
	BOOL       auto_reopen  : 1;
	BOOL       auto_request : 1;
	AMessage   work_msg;

	inline void init(BOOL reopen, BOOL req_work) {
		status = AObject_Invalid;
		RB_CLEAR_NODE(&cm_node);
		manager = NULL;
		on_open = NULL;
		on_close = NULL;
		on_erase = NULL;

		AOperatorTimeinit(&main_asop);
		main_msg.init();
		main_msg.done = NULL;

		active = 0;
		last_opened = FALSE;
		owner_thread = FALSE;
		auto_reopen = reopen;
		auto_request = req_work;
		work_msg.init();
		work_msg.done = NULL;
	}
	inline void exit() {
		assert(RB_EMPTY_NODE(&cm_node));
		assert(main_asop.callback == NULL);
	}
	inline void shutdown() {
		status = AObject_Abort;
		close(this, NULL);
		release2();
	}
};



#endif
