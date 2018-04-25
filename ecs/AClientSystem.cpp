#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEvent.h"
#include "AClientSystem.h"


static ASystem::Result* check_one(ASystemManager *sm, AClientComponent *c, DWORD cur_tick)
{
	int diff = int(cur_tick - c->_main_tick);
	if (diff < 0)
		diff = 0;

	ASystem::Status status = ASystem::Runnable;
	switch (c->_status)
	{
	case AClientComponent::Invalid:
		if (c->_busy_count != 0)
			return NULL;
		if ((c->_main_tick != 0) && (diff < c->_tick_reopen))
			return NULL;

		c->_main_tick = cur_tick;
		c->_main_abort = false;
		c->_check_heart = AClientComponent::HeartChecking;
		break;

	case AClientComponent::Opening:
		if (diff > c->_tick_abort) {
			status = ASystem::Aborting;
			break;
		}
		return NULL;

	case AClientComponent::Opened:
		if (diff > c->_tick_abort) {
			status = ASystem::Aborting;
			break;
		}

		if (c->_main_abort) {
			if (c->_busy_count != 0)
				return NULL;
			c->_status = AClientComponent::Closing;
			break;
		}

		if ((diff < c->_tick_heart)
		 || (c->heart == NULL)
		 || (c->_check_heart != AClientComponent::HeartNone))
			return NULL;
		c->_check_heart = AClientComponent::HeartChecking;
		break;

	case AClientComponent::Closing:
		if (c->_busy_count == 0)
			break;
		if (diff > c->_tick_abort) {
			status = ASystem::Aborting;
			break;
		}
		return NULL;

	case AClientComponent::Closed:
		if (c->_busy_count != 0)
			return NULL;
		if (!c->_auto_reopen) {
			if (!c->_auto_remove)
				return NULL;

			c->_entity->addref();
			sm->_all_entities->_pop(sm->_all_entities, c->_entity);

			if (!c->_sys_node.empty()) {
				c->_sys_node.leave();
				c->_entity->release();
			}
			c->use(1);
			c->_run_result.status = ASystem::Runnable;
			return &c->_run_result;
		}
		c->_status = AClientComponent::Invalid;
		c->_main_tick = cur_tick;
		c->_main_abort = false;
		return NULL;

	default: assert(0); return NULL;
	}

	if (status == ASystem::Aborting) {
		c->_main_tick = cur_tick;
		c->_main_abort = true;
		if (c->abort == NULL)
			return NULL;
		c->_entity->addref();
		c->use(1);
		c->_abort_result.status = ASystem::Aborting;
		return &c->_abort_result;
	}

	c->_entity->addref();
	c->use(1);
	c->_run_result.status = ASystem::Runnable;
	return &c->_run_result;
}

static ASystem::Result* client_check(ASystemManager *sm, AEntity *e, DWORD cur_tick)
{
	AClientComponent *c;
	if (e->get(&c) == NULL)
		return NULL; //NotNeed;
	return check_one(sm, c, cur_tick);
}

static int client_run_internal(ASystem::Result *r, int result)
{
	AClientComponent *c = container_of(r, AClientComponent, _run_result);
	TRACE("%s(%p, %d): status = %s, busy_count = %d, result = %d.\n",
		c->_entity->_module->module_name, c->_entity, c->_entity->_refcount,
		c->status_name(), c->_busy_count, result);

	switch (c->_status)
	{
	case AClientComponent::Invalid:
		c->_status = AClientComponent::Opening;
		result = c->open(c);
		if (result == 0)
			return 0;

	case AClientComponent::Opening:
		if (result >= 0) {
			c->_main_tick = GetTickCount();
			c->_status = AClientComponent::Opened;
			r->manager->emit_by_name("on_client_opened", c);

			c->_last_opened = true;
			if (c->_open_heart && (c->heart != NULL)) {
				c->_check_heart = AClientComponent::HeartChecking;
			} else {
				c->_check_heart = AClientComponent::HeartNone;
				break;
			}
		}

	case AClientComponent::Opened:
		if (result >= 0) {
			if (c->_check_heart == AClientComponent::HeartChecking) {
				c->_check_heart = AClientComponent::HeartCheckDone;
				result = c->heart(c);
				if (result == 0)
					return 0;
			}
		}
		if (result >= 0) {
			c->_main_tick = GetTickCount();
			assert(c->_check_heart == AClientComponent::HeartCheckDone);
			c->_check_heart = AClientComponent::HeartNone;
			break;
		}
		c->_main_abort = true;
		if (c->abort) c->abort(c);
		c->_status = AClientComponent::Closing;
		if (c->_busy_count != 1)
			break;

	case AClientComponent::Closing:
		c->_status = AClientComponent::Closed;
		result = c->close(c);
		if (result == 0)
			return 0;

	case AClientComponent::Closed:
		r->manager->emit_by_name("on_client_closed", c);
		c->_last_opened = false;
		break;

	default: assert(0); return result;
	}

	r->status = ASystem::NotNeed;
	c->use(-1);
	c->_entity->release();
	return result;
}

static void* client_run_thread(void *p)
{
	ASystem::Result *r = (ASystem::Result*)p;
	client_run_internal(r, 0);
	return NULL;
}

static int client_run(ASystem::Result *r, int result)
{
	AClientComponent *c = container_of(r, AClientComponent, _run_result);
	if (c->_owner_thread
	 && ((c->_status == AClientComponent::Invalid) || (c->_status == AClientComponent::Closing)))
	{
		assert(result == 0);
		pthread_post(r, &client_run_thread);
	} else {
		result = client_run_internal(r, result);
	}
	return result;
}

static int client_abort(ASystem::Result *r)
{
	AClientComponent *c = container_of(r, AClientComponent, _abort_result);
	int result = c->abort(c);
	c->use(-1);

	TRACE("%s(%p, %d): status = %s, busy_count = %d.\n",
		c->_entity->_module->module_name, c->_entity, c->_entity->_refcount,
		c->status_name(), c->_busy_count);
	c->_entity->release();
	return result;
}

//////////////////////////////////////////////////////////////////////////
static LIST_HEAD(g_com_list);

static int reg_client(AEntity *e)
{
	AClientComponent *c;
	if ((e->get(&c) == NULL) || !c->_sys_node.empty())
		return 0;
	c->_entity->addref();
	g_com_list.push_back(&c->_sys_node);
	return 1;
}

static int unreg_client(AEntity *e)
{
	AClientComponent *c;
	if ((e->get(&c) == NULL) || c->_sys_node.empty())
		return 0;
	c->_sys_node.leave();
	c->_entity->release();
	return 1;
}

static int clear_all(bool abort)
{
	int count = 0;
	while (!g_com_list.empty()) {
		AClientComponent *c = list_pop_front(&g_com_list, AClientComponent, _sys_node);
		if (abort && c->abort) c->abort(c);
		c->_entity->release();
		count ++;
	}
	return count;
}

static int check_all(ASystemManager *sm, list_head *results, DWORD cur_tick)
{
	int count = 0;
	list_for_each2(c, &g_com_list, AClientComponent, _sys_node) {
		ASystem::Result *r = check_one(sm, c, cur_tick);
		if (r != NULL) {
			extern ASystem AClientSystem;
			r->system = &AClientSystem;
			results->push_back(&r->node);
			count ++;
		}
	}
	return count;
}

static int client_com_null(AClientComponent *c)
{
	AModule *m = c->_entity->_module;
	TRACE2("%s(%s): no implement.\n", m->module_name, m->class_name);
	return -ENOSYS;
}

static int client_com_create(AObject **object, AObject *parent, AOption *option)
{
	AClientComponent *c = (AClientComponent*)object;
	c->init(c->name());
	c->init2();
	if (parent != NULL)
		((AEntity*)parent)->push(c);
	if (option != NULL) {
		c->_tick_reopen = option->getInt("tick_reopen", c->_tick_reopen/1000)*1000;
		c->_tick_heart  = option->getInt("tick_heart",  c->_tick_heart/1000)*1000;
		c->_tick_abort  = option->getInt("tick_abort",  c->_tick_abort/1000)*1000;
		c->_owner_thread = !!option->getInt("owner_thread", c->_owner_thread);
		c->_open_heart   = !!option->getInt("open_heart",   c->_open_heart);
		c->_auto_reopen  = !!option->getInt("auto_reopen",  c->_auto_reopen);
		c->_auto_remove  = !!option->getInt("auto_remove",  c->_auto_remove);
	}
	// set by implement module
	c->open = &client_com_null;
	c->close = &client_com_null;
	return 1;
}

ASystem AClientSystem = { {
	"ASystem",
	AClientComponent::name(),
	sizeof(AClientComponent),
	NULL, NULL,
	&client_com_create, NULL,
},
	&reg_client,
	&unreg_client,
	&clear_all,
	&check_all,
	&client_check,
	&client_run,
	&client_abort,
};

static int reg_cs = AModuleRegister(&AClientSystem.module);
