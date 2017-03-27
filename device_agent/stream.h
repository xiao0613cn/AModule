#ifndef _AMODULE_STREAM_H_
#define _AMODULE_STREAM_H_


typedef struct AStream {
	AObject          object;
	AObject_Status   status;
	struct ADevice  *device;

	int              chan_id;
	int              stream_id;
	struct list_head tx_entry;

	AMessage         tx_msg;
	pthread_mutex_t  tx_mutext;
	struct list_head client_list;
	inline void      tx_lock() { pthread_mutex_lock(&tx_mutext); }
	inline void      tx_unlock() { pthread_mutex_unlock(&tx_mutext); }

	inline void init() {
		status = AObject_Invalid;
		device = NULL;
		INIT_LIST_HEAD(&tx_entry);

		chan_id = 0;
		stream_id = 0;
		tx_msg.init();
		pthread_mutex_init(&tx_mutext, NULL);
		INIT_LIST_HEAD(&client_list);
	}
	inline void exit() {
		if (device != NULL)
			device->object.release2();
		pthread_mutex_destroy(&tx_mutext);
		assert(list_empty(&tx_entry));
		assert(list_empty(&client_list));
	}
} AStream;



#endif
