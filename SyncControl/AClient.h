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
	DWORD      main_tick;
	int        main_run(int result);
	void  main_end(AObject_Status s) {
		main_msg.done = NULL;
		if (s != AObject_Invalid)
			status = s;
		main_asop.done = NULL;
		release2();
	}

	unsigned   main_abort   : 1;
	unsigned   last_opened  : 1;
	unsigned   owner_thread : 1;
	unsigned   auto_reopen  : 1;
	unsigned   auto_request : 1;
	AMessage   work_msg;

	void  init(BOOL reopen, BOOL req_work) {
		status = AObject_Invalid;
		RB_CLEAR_NODE(&cm_node);
		manager = NULL;
		on_open = NULL;
		on_close = NULL;
		on_erase = NULL;

		main_asop.done = NULL;
		main_msg.done = NULL;
		main_tick = 0;

		main_abort = FALSE;
		last_opened = FALSE;
		owner_thread = FALSE;
		auto_reopen = reopen;
		auto_request = req_work;
		work_msg.init();
		work_msg.done = NULL;
	}
	void  exit() {
		assert(RB_EMPTY_NODE(&cm_node));
		assert(main_asop.done == NULL);
	}
	void  shutdown() {
		main_abort = TRUE;
		close(NULL);
		release2();
	}
};



#endif
