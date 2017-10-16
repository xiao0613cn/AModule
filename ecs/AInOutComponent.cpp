#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AInOutComponent.h"


AInOutComponent::com_module InOutModule = { {
	AInOutComponent::name(),
	AInOutComponent::name(), },
	&AInOutComponent::_do_post,
	&AInOutComponent::_inmsg_done,
	&AInOutComponent::_outmsg_done,
};

static auto_reg_t reg(InOutModule.module);
