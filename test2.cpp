#include "stdafx.h"
//#include <Windows.h>
//#include <crtdbg.h>
#include "base/base.h"
#include "base/list.h"
#include "base/rbtree.h"
#include "base/AEntity.h"
#include "base/spinlock.h"
#include "base/AEvent.h"

rb_tree_define(AEntity, _node, AEntity*, AEntityCmp)

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
		defer(AEntity*, {
			if (_value != NULL)
				_value->_self->release2();
		}) ac2(NULL);

		*ac = 5;
		ac2 = &e;
		//ac2->_self->addref();
	}

	null_lock_helper l;
	EMTmpl<null_lock_helper> emh; emh.init(&em, &l);
	emh.get(&e, "1");

	_CrtDumpMemoryLeaks();
	return 0;
}
