#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEntity.h"
#include "AClientSystem.h"


static ASystem::Result* AClientSystem_exec_check(AEntity *e, DWORD cur_tick)
{
	AClientComponent *c = e->_get<AClientComponent>();
	if (c == NULL)
		return NULL; //NotNeed;

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
		c->_last_opened = false;
		c->_check_heart = AClientComponent::HeartChecking;
		break;

	case AClientComponent::Opening:
	case AClientComponent::Opened:
		if (diff > c->_tick_abort) {
			c->_main_tick = cur_tick;
			c->_main_abort = true;
			if (c->abort == NULL)
				return NULL;
			status = ASystem::Aborting;
			break;
		}

		if (c->_main_abort || (c->_status == AClientComponent::Opening)) {
			if (c->_busy_count != 0)
				return NULL;
			c->_status = AClientComponent::Closing;
			break;
		}

		if ((diff < c->_tick_heart)
		 || (c->_check_heart != AClientComponent::HeartNone))
			return NULL;
		c->_check_heart = AClientComponent::HeartChecking;
		break;

	case AClientComponent::Closing:
		if (c->_busy_count != 0)
			return NULL;
		break;

	case AClientComponent::Closed:
		if (c->_busy_count != 0)
			return NULL;
		if (c->_auto_reopen) {
			c->_status = AClientComponent::Invalid;
			c->_main_tick = cur_tick;
			c->_main_abort = false;
			return NULL;
		}
		c->_entity->_pop(c);
		break;

	default: assert(0); return NULL;
	}

	if (status == ASystem::Aborting) {
		c->_self->addref();
		c->use(1);
		c->_abort_result.status = ASystem::Aborting;
		return &c->_abort_result;
	}

	if (c->_status != AClientComponent::Closed)
		c->_self->addref();
	c->use(1);
	c->_run_result.status= ASystem::Runnable;
	return &c->_run_result;
}

static int AClientSystem_exec_run(AEntity *e, ASystem::Result *r, int result)
{
	AClientComponent *c = container_of(r, AClientComponent, _run_result);
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
				break;
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
		break;

	default: assert(0); return result;
	}

	c->use(-1);
	c->_self->release();
	return result;
}

static int AClientSystem_exec_abort(AEntity *e, ASystem::Result *r)
{
	AClientComponent *c = container_of(r, AClientComponent, _abort_result);
	int result = c->abort(c);
	c->use(-1);
	c->_self->release();
	return result;
}

ASystem AClientSystem = { {
	"ASystem",
	"AClientSystem", },
	&AClientSystem_exec_check,
	&AClientSystem_exec_run,
	&AClientSystem_exec_abort,
};

static auto_reg_t reg(AClientSystem.module);
