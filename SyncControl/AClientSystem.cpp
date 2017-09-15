#include "stdafx.h"
#include "../base/AModule_API.h"
#include "../base/AEntity.h"
#include "AClientSystem.h"

//////////////////////////////////////////////////////////////////////////
static inline int AClientComponentCmp(AClientComponent *key, AClientComponent *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
//rb_tree_define(AClientComponent, _system_node, AClientComponent*, AClientComponentCmp)
#if 0
void AClientSystem::init()
{
	//INIT_RB_ROOT(&_client_map);
	//_client_count = 0;

	_exec_tick = 200;
	_exec_asop.timer();
	_exec_asop.done = &AsopDone(AClientSystem, _exec_asop, _execute);
	_exec_abort = false;
	_exec_thread = NULL;
	_exec_last = NULL;
}

static int AClientSystemExecuteOne(AOperator *asop, int result)
{
	AClientComponent *c = container_of(asop, AClientComponent, _system_asop);
	result = c->_system->_exec_one(c, result);
	if (result != 0)
		c->_self->release2();
	return result;
}

static void* AClientComponentExecuteThread(void *p)
{
	AClientComponent *c = (AClientComponent*)p;
	c->_system_asop.done2(1);
	return NULL;
}

void AClientSystem::_exec_post(AClientComponent *c, bool addref)
{
	if (addref)
		c->_self->addref();
	c->_system_asop.ao_thread = NULL;
	c->_system_asop.done = &AClientSystemExecuteOne;

	if (c->_owner_thread) {
		pthread_t tid = pthread_null;
		pthread_create(&tid, NULL, &AClientComponentExecuteThread, c);
		pthread_detach(tid);
	} else {
		c->_system_asop.post(_exec_thread);
	}
}
#endif
/*
bool AClientSystem::_push(AClientComponent *c)
{
	bool valid = ((c->_system == NULL) && RB_EMPTY_NODE(&c->_system_node));
	if (valid)
		valid = (rb_insert_AClientComponent(&_client_map, c, c) == NULL);
	if (valid) {
		++_client_count;
		//c->_self->addref();
		c->_system = this;
		c->_system_asop.ao_thread = NULL;
		c->_system_asop.done = &AClientSystemExecuteOne;
	} else {
		assert(0);
	}
	return valid;
}

bool AClientSystem::_pop(AClientComponent *c)
{
	bool valid = ((c->_system == this) && !RB_EMPTY_NODE(&c->_system_node));
	if (valid) {
		rb_erase(&c->_system_node, &_client_map);
		RB_CLEAR_NODE(&c->_system_node);
		--_client_count;
	} else {
		assert(0);
	}
	//if (valid) c->_self->release2();
	return valid;
}

int AClientSystem::_execute(int result)
{
	if (_exec_abort)
		_exec_asop.done = NULL;
	else
		_exec_asop.delay(_exec_thread, _exec_tick, TRUE);
	return result;
}
*/
enum ASystem::Result AClientSystem::_exec_check(AClientComponent *c, DWORD cur_tick)
{
	int diff = int(cur_tick - c->_main_tick);
	if (diff < 0)
		diff = 0;

	switch (c->_status)
	{
	case AClientComponent::Invalid:
		if (c->_busy_count != 0)
			return Invalid;
		if ((c->_main_tick != 0) && (diff < c->_tick_reopen))
			return Invalid;

		c->_main_tick = cur_tick;
		c->_main_abort = false;
		c->_last_opened = false;
		c->_check_heart = AClientComponent::HeartChecking;
		break;

	case AClientComponent::Opening:
	case AClientComponent::Opened:
		if (diff > c->_tick_abort) {
			c->_main_tick = cur_tick;
			c->_main_abort = true;
			return Abort;
		}

		if (c->_main_abort || (c->_status == AClientComponent::Opening)) {
			if (c->_busy_count != 0)
				return Invalid;
			c->_status = AClientComponent::Closing;
			break;
		}

		if ((diff < c->_tick_heart)
		 || (c->_check_heart != AClientComponent::HeartNone))
			return Invalid;

		c->_check_heart = AClientComponent::HeartChecking;
		break;

	case AClientComponent::Closing:
		if (c->_busy_count != 0)
			return Invalid;
		break;

	case AClientComponent::Closed:
		if (c->_busy_count != 0)
			return Invalid;
		if (!c->_auto_reopen) {
			c->_status = AClientComponent::Invalid;
			c->_main_tick = cur_tick;
			c->_main_abort = false;
			return Invalid;
		}
		//_pop(c);
		break;

	default: assert(0); return Invalid;
	}
	c->use(1);
	return Success;
}

int AClientSystem::_exec_one(AClientComponent *c, int result)
{
	if (result == 0)
		result = 1;

	switch (c->_status)
	{
	case AClientComponent::Invalid:
		if (result > 0) {
			c->_status = AClientComponent::Opening;
			result = c->open(c);
			if (result == 0)
				return 0;
		}

	case AClientComponent::Opening:
		if (result > 0) {
			c->_main_tick = GetTickCount();
			c->_status = AClientComponent::Opened;
			c->_last_opened = true;
			if (c->_open_heart) {
				c->_check_heart = AClientComponent::HeartChecking;
			} else {
				c->_check_heart = AClientComponent::HeartNone;
				c->use(-1);
				return 1;
			}
		}

	case AClientComponent::Opened:
		if (result > 0) {
			if (c->_check_heart == AClientComponent::HeartChecking) {
				c->_check_heart = AClientComponent::HeartCheckDone;
				result = c->heart(c);
				if (result == 0)
					return 0;
			}
		}
		if (result > 0) {
			c->_main_tick = GetTickCount();
			assert(c->_check_heart == AClientComponent::HeartCheckDone);
			c->_check_heart = AClientComponent::HeartNone;
		} else {
			c->abort(c);
			c->_status = AClientComponent::Closing;
		}
		c->use(-1);
		return result;

	case AClientComponent::Closing:
		c->_status = AClientComponent::Closed;
		result = c->close(c);
		if (result == 0)
			return 0;

	case AClientComponent::Closed:
		c->use(-1);
		return result;

	default: assert(0); return result;
	}
}
