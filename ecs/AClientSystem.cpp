#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEntity.h"
#include "AClientSystem.h"


static ASystem::Result* check_one(AClientComponent *c, DWORD cur_tick)
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
			return NULL;
		}
		c->_status = AClientComponent::Invalid;
		c->_main_tick = cur_tick;
		c->_main_abort = false;
		return NULL;
		//c->_entity->_pop(c);
		//if (!c->_sys_node.empty())
		//	c->_sys_node.leave();
		//break;

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

	if (c->_status != AClientComponent::Closed)
		c->_entity->addref();
	c->use(1);
	c->_run_result.status= ASystem::Runnable;
	return &c->_run_result;
}

static ASystem::Result* client_check(AEntity *e, DWORD cur_tick)
{
	AClientComponent *c; e->_get(&c);
	if (c == NULL)
		return NULL; //NotNeed;
	return check_one(c, cur_tick);
}

static int client_run(ASystem::Result *r, int result)
{
	AClientComponent *c = container_of(r, AClientComponent, _run_result);
	TRACE("%s(%p, %d): status = %d, busy_count = %d, result = %d.\n",
		c->_entity->_module->module_name, c->_entity, c->_entity->_refcount,
		c->_status, c->_busy_count, result);

	switch (c->_status)
	{
	case AClientComponent::Invalid:
		c->_status = AClientComponent::Opening;
		result = c->open(c);
		if (result == 0)
			return 0;

	case AClientComponent::Opening:
		if (result > 0) {
			c->_main_tick = GetTickCount();
			c->_status = AClientComponent::Opened;
			r->manager->emit(r->manager, "on_client_opened", c);

			c->_last_opened = true;
			if (c->_open_heart) {
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
		} else {
			if (c->abort) c->abort(c);
			c->_status = AClientComponent::Closing;
		}
		break;

	case AClientComponent::Closing:
		c->_status = AClientComponent::Closed;
		result = c->close(c);
		if (result == 0)
			return 0;

	case AClientComponent::Closed:
		r->manager->emit(r->manager, "on_client_closed", c);
		c->_last_opened = false;
		break;

	default: assert(0); return result;
	}

	r->status = ASystem::NotNeed;
	c->use(-1);
	c->_entity->release();
	return result;
}

static int client_abort(ASystem::Result *r)
{
	AClientComponent *c = container_of(r, AClientComponent, _abort_result);
	int result = c->abort(c);
	c->use(-1);

	TRACE("%s(%p, %d): status = %d, busy_count = %d.\n",
		c->_entity->_module->module_name, c->_entity, c->_entity->_refcount,
		c->_status, c->_busy_count);
	c->_entity->release();
	return result;
}

//////////////////////////////////////////////////////////////////////////
static LIST_HEAD(g_com_list);

static int reg_client(AEntity *e)
{
	AClientComponent *c; e->_get(&c);
	if ((c == NULL) || !c->_sys_node.empty())
		return 0;
	c->_entity->addref();
	g_com_list.push_back(&c->_sys_node);
	return 1;
}

static int unreg_client(AEntity *e)
{
	AClientComponent *c; e->_get(&c);
	if ((c == NULL) || c->_sys_node.empty())
		return 0;
	c->_sys_node.leave();
	c->_entity->release();
	return 1;
}

static int check_all(list_head *results, DWORD cur_tick)
{
	int count = 0;
	list_for_each2(c, &g_com_list, AClientComponent, _sys_node) {
		ASystem::Result *r = check_one(c, cur_tick);
		if (r != NULL) {
			extern ASystem AClientSystem;
			r->system = &AClientSystem;
			results->push_back(&r->node);
			count ++;
		}
	}
	return count;
}

ASystem AClientSystem = { {
	ASystem::class_name(),
	"AClientSystem", },
	&reg_client,
	&unreg_client,
	&check_all,
	&client_check,
	&client_run,
	&client_abort,
};

static int reg_cs = AModuleRegister(&AClientSystem.module);
