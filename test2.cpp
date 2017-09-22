#include "stdafx.h"
//#include <Windows.h>
//#include <crtdbg.h>
#include "base/AModule_API.h"
#include "base/spinlock.h"
#include "ecs/AEntity.h"
#include "ecs/AEvent.h"
#include "ecs/ASystem.h"

rb_tree_define(AEntity, _manager_node, AEntity*, AEntityCmp)
rb_tree_define(AReceiver, _manager_node, const char*, AReceiverCmp)
rb_tree_define(AReceiver2, _manager_node2, long, AReceiver2Cmp)

static int
on_event(AReceiver *r, AEvent *e) {
	TRACE("r->name = %s, r->index = %d, e->name = %s, e->index = %d.\n",
		r->_name, r->_index, e->_name, e->_index);
	return 0;
}

int main()
{
	AEntity e; e.init(NULL);
	AEntityManager em; em.init();
	em._push(&e);

	//e._append(new AComponent(NULL));
	//e._append(new AComponent(NULL));
	{
		defer(int*, delete [] _value) ac(new int[5]);

		for (int ix = 0; ix < 5; ++ix) {
			ac._value[ix] = ix;
			//ac->[5] = 1;
		}
	}
	{
		defer(int*, delete _value) ac(new int);
		//defer(AEntity*, {
		//	if (_value != NULL)
		//		_value->_self->release();
		//}) ac2(NULL);

		*ac = 5;
		//ac2 = &e;
		//ac2->_self->addref();
	}

	null_lock_helper l;
	//EMTmpl<null_lock_helper> emh; emh.init(&em, &l);
	//emh.get(&e, "1");

	ASlice<char> *buf = ASlice<char>::create(32*1024);
	APlane<char>::type *plane = APlane<char>::type::create(16);

	AEvent ev; ev.init("test_event", 1);

	AReceiver r; r.init(NULL);
	r._name = "test_event";
	r._index = 2;
	r.receive = &on_event;

	AEventManager evm; evm.init();
	evm._subscribe(&r);
	evm._emit(&ev);

	ASystem *s = ASystem::create("AClientSystem");


	_CrtDumpMemoryLeaks();
	return 0;
}
