#include "stdafx.h"
#include "AClientManager.h"
#include "AClient.h"


static inline int client_cmp(AClient *key, AClient *c)
{
	if (key == c) return 0;
	return (key > c) ? 1 : -1;
}
rb_tree_define(AClient, cm_node, AClient*, client_cmp)

AMODULE_API void CM_init(AClientManager *cm)
{
	INIT_RB_ROOT(&cm->client_map);
	pthread_mutex_init(&cm->client_mutex, NULL);

	cm->timer_reconnect = 30*1000;
	cm->timer_offline = 30*1000;
	cm->timer_heart = 10*1000;
	cm->timer_check = 1*1000;

	AOperatorTimeinit(&cm->check_asop);
	cm->check_thread = NULL;
	cm->check_post_num = 8;
	cm->check_last_client = NULL;
	cm->check_running = FALSE;
}

AMODULE_API void CM_start(AClientManager *cm)
{
	cm->check_running = TRUE;
	cm->setCppAsopCB(AClientManager, check_asop, check_run);

	AThreadPost(cm->check_thread, &cm->check_asop, TRUE);
}

AMODULE_API void CM_stop(AClientManager *cm)
{
	cm->check_running = FALSE;
	AOperatorSignal(&cm->check_asop, cm->check_thread, TRUE);

	while (cm->check_asop.callback != NULL)
		Sleep(50);
}

AMODULE_API void CM_exit(AClientManager *cm)
{
	assert(RB_EMPTY_ROOT(&cm->client_map));
	pthread_mutex_destroy(&cm->client_mutex);

	assert(cm->check_asop.callback == NULL);
}

static void on_erase_null(AClient*) { }

AMODULE_API BOOL CM_add_internal(AClientManager *cm, struct AClient *c)
{
	AClient *old = rb_insert_AClient(&cm->client_map, c, c);
	if (old == NULL) {
		if (c->on_erase == NULL)
			c->on_erase = &on_erase_null;
		c->manager = cm;
	}
	return (old == NULL);
}

static inline void erase_client(AClientManager *cm, AClient *c)
{
	rb_erase(&c->cm_node, &cm->client_map);
	RB_CLEAR_NODE(&c->cm_node);
	c->on_erase(c);
}

AMODULE_API BOOL CM_del_internal(AClientManager *cm, AClient *c)
{
	c = rb_find_AClient(&cm->client_map, c);
	if (c != NULL) {
		erase_client(cm, c);
	}
	return (c != NULL);
}

int AClientManager::check_run(int result)
{
	if (check_running && (result >= 0)) {
		check_status();
	}

	if (check_running && (result >= 0)) {
		if (check_last_client != NULL)
			AThreadPost(check_thread, &check_asop, TRUE);
		else
			AOperatorTimewait(&check_asop, check_thread, timer_check, TRUE);
	} else {
		check_abort();
	}
	return result;
}

AMODULE_API void CM_check_status(AClientManager *cm) { cm->check_status(); }

void AClientManager::check_status()
{
	DWORD tick = GetTickCount();
	int ix = 0;

	client_lock();
	while (check_running) {
		AClient *c = check_last_client = rb_upper_AClient(&client_map, check_last_client);
		if (c == NULL)
			break;

		int diff = tick - c->active;
		if (diff < 0)
			diff = 0;

		int do_shutdown = 0;
		switch (c->status)
		{
		case AObject_Opening:
		case AObject_Opened:
			if (diff >= timer_offline)
				do_shutdown = 1; // shutdown
			else if ((c->status != AObject_Opened)
			      || (c->main_asop.callback != NULL)
			      || (diff < timer_heart))
				continue;
			c->addref(); // check heart
			break;

		case AObject_Abort:
			if ((c->main_asop.callback != NULL)
			 || (c->refcount > 1))
				continue;
			if (!c->auto_reopen) // close
				erase_client(this, c);
			else
				c->addref();
			break;

		case AObject_Closed:
			if (!c->auto_reopen) {
				erase_client(this, c);
				do_shutdown = 2;
				break;
			}
			c->status = AObject_Invalid; // re-open

		case AObject_Invalid:
			if ((c->main_asop.callback != NULL)
			 || (c->refcount > 1)
			 || (c->active != 0 && diff < timer_reconnect))
				continue;
			c->addref(); // open
			break;

		default: assert(FALSE);
		case AObject_Closing:
			if (!c->auto_reopen) {
				erase_client(this, c);
				do_shutdown = 2;
				break;
			}
			continue;
		}
		client_unlock();

		if (do_shutdown == 2) {
			c->release2();
		} else if (do_shutdown) {
			c->shutdown();
		} else {
			CM_check_post(this, c, FALSE);
			if (++ix >= check_post_num)
				return;
		}

		tick = GetTickCount();
		client_lock();
	}
	client_unlock();
}

AMODULE_API void CM_check_abort(AClientManager *cm) { cm->check_abort(); }

void AClientManager::check_abort()
{
	client_lock();
	for (struct rb_node *node = rb_first(&client_map); node != NULL; node = rb_next(node)) {
		AClient *c = rb_entry(node, AClient, cm_node);
		c->on_erase(c);
	}

	struct rb_root quit_tree = client_map;
	INIT_RB_ROOT(&client_map);
	client_unlock();

	struct rb_node *node = rb_first(&quit_tree);
	while (node != NULL)
	{
		AClient *c = rb_entry(node, AClient, cm_node);
		node = rb_next(node);

		rb_erase(&c->cm_node, &quit_tree);
		RB_CLEAR_NODE(&c->cm_node);
		c->shutdown();
	}
	check_asop.callback = NULL;
}

static void* AClient_run(void *p)
{
	AClient *c = (AClient*)p;
	c->main_run(1);
	return NULL;
}

AMODULE_API void CM_check_post(AClientManager *cm, struct AClient *c, BOOL addref)
{
	if (c->main_asop.callback == NULL)
		c->setCppAsopCB(AClient, main_asop, main_run);

	if (addref)
		c->addref();

	if (c->owner_thread) {
		pthread_t thr = pthread_null;
		pthread_create(&thr, NULL, AClient_run, c);
		pthread_detach(thr);
	} else {
		AThreadPost(cm->check_thread, &c->main_asop, TRUE);
	}
}

//////////////////////////////////////////////////////////////////////////
int AClient::main_run(int result)
{
	if (result == 0)
		result = 1;
	while (result != 0) {
		if ((result < 0) && (status < AObject_Abort))
			status = AObject_Abort;

		if (status != AObject_Opened) {
			active = GetTickCount();
			TRACE("%s: status = %s, result = 0x%X.\n",
				module->module_name, StatusName(status), result);
		}

		switch (status)
		{
		case AObject_Invalid:
			setCppMsgDone(AClient, main_msg, main_run);

			status = AObject_Opening;
			result = open(this, &main_msg);
			break;

		case AObject_Opening:
			status = AObject_Opened;
			last_opened = TRUE;
			if (on_open != NULL)
				on_open(this);

			if (auto_request && (work_msg.done != NULL)) {
				addref();
				result = request(this, 1, &work_msg);
				if (result != 0)
					work_msg.done(&work_msg, result);
			}
			main_end(AObject_Invalid);
			return result;

		case AObject_Opened: // check heart
			if (main_msg.done == NULL) {
				setCppMsgDone(AClient, main_msg, main_run);

				result = request(this, 0, &main_msg);
				break;
			} else {
				active = GetTickCount();
				main_end(AObject_Invalid); // don't set status
				return result;
			}

		case AObject_Abort:
			if (last_opened) {
				last_opened = FALSE;
				if (on_close != NULL)
					on_close(this);
			}
			if (this->refcount > 2) {
				TRACE("%s: refcount = %d, wait release by other using...\n",
					module->module_name, refcount);
				main_end(AObject_Invalid);
				return result;
			}
			setCppMsgDone(AClient, main_msg, main_run);

			status = AObject_Closing;
			result = close(this, &main_msg);
			break;

		case AObject_Closing:
		case AObject_Closed:
			main_end(AObject_Closed);
			return result;

		default: assert(FALSE); return result;
		}
	}
	assert(result == 0);
	return 0;
}

/*
int AClient::work_run(int result)
{
	if (result == 0)
		result = 1;
	while (result > 0) {
		work_msg.init();

		result = request(this, 1, &work_msg);
		if (result == 0)
			return 0;
	}
	TRACE("status = %s, result = %x", StatusName(status), result);
	release2();
}
*/