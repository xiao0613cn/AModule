#ifndef AMODULE_SYNCREQUEST_H
#define AMODULE_SYNCREQUEST_H


struct SyncReqHelper {
	void  lock_req(SyncRequest *req);
	int   check_req(SyncRequest *req, AObject **obj, AMessage *msg);
	void  unlock_req(SyncRequest *req);

	int   msg_req(SyncRequest *req, AMessage *msg);
	void  msg_done(SyncRequest *req, AMessage *msg, int result);
};


template <typename TObject = SyncReqHelper>
struct SyncRequest
{
	TObject            *p_this;
	int                 req_ix;
	struct list_head    req_list;

	AObject            *obj_proxy;
	AMessage            msg_proxy;

	inline void init(TObject *p, int ix) {
		p_this = p;
		req_ix = ix;
		INIT_LIST_HEAD(&req_list);
	}
	inline void exit() {
		assert(list_empty(&req_list));
	}

	inline int post(AMessage *msg) {
		AObject *obj = NULL;

		p_this->lock_req(this);
		int result = p_this->check_req(this, &obj, msg);
		if (result >= 0) {
			result = list_empty(&req_list);
			list_add_tail(&msg->entry, &req_list);
		}
		if (result > 0) {
			obj->addref();
			obj_proxy = obj;
		} else {
			obj = NULL;
		}
		p_this->unlock_req(this);

		if (result < 0) {
			p_this->msg_done(this, msg, result);
			msg->done(msg, result);
			return result;
		}
		if (obj == NULL)
			return 0;

		msg_proxy.init(msg);
		setCppMsgDone(SyncRequest, msg_proxy, on_post);

		result = obj->request(obj, req_ix, &msg_proxy);
		if (result != 0)
			on_post(result);
		return 0;
	}

	inline int on_post(int result) {
		if (result == 0)
			result = 1;

		AObject *obj = obj_proxy;
		for (;;) {
			AMessage *next_msg = NULL;
			int next_result = 0;

			p_this->lock_req(this);
			AMessage *msg = list_pop_front(&req_list, AMessage, entry);

			if (!list_empty(&req_list)) {
				AMessage *m2 = list_first_entry(&req_list, AMessage, entry);

				next_result = p_this->check_req(this, &obj, m2);
				next_msg = m2;
			}
			p_this->unlock_req(this);

			p_this->msg_done(this, msg, result);
			msg->done(msg, result);
			if (next_msg == NULL)
				break;

			if (next_result < 0) {
				result = next_result;
				continue;
			}

			msg_proxy.init(next_msg);
			setCppMsgDone(SyncRequest, msg_proxy, on_post);

			result = obj->request(obj, req_ix, &msg_proxy);
			if (result == 0)
				return 0;
		}
		obj->release2();
		return result;
	}
};
#define to_req(m) container_of(m, SyncRequest, msg)


#endif //AMODULE_SYNCREQUEST_H
