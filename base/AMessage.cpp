#include "stdafx.h"
#include "AModule_API.h"


AMODULE_API AMessage*
AMsgDispatch(struct list_head *notify_list, AMessage *from, struct list_head *quit_list)
{
	AMessage *pos;
	list_for_each_entry(pos, notify_list, AMessage, entry)
	{
		if ((from->type != AMsgType_Unknown)
		 && (pos->type != AMsgType_Unknown)
		 && (pos->type != from->type))
			continue;

		pos->init(from);
		int result = pos->done(pos, 0);
		if (result == 0)
			continue;

		if (result > 0) {
			list_del_init(&pos->entry);
			return pos;
		}

		pos = list_entry(pos->entry.prev, AMessage, entry);
		list_move_tail(pos->entry.next, quit_list);
	}
	return NULL;
}

AMODULE_API int
AMsgDispatch2(pthread_mutex_t *mutex, struct list_head *notify_list, AMessage *from)
{
	struct list_head quit_list;
	INIT_LIST_HEAD(&quit_list);

	pthread_mutex_lock(mutex);
	AMessage *msg = AMsgDispatch(notify_list, from, &quit_list);
	pthread_mutex_unlock(mutex);

	if (msg != NULL) {
		msg->done(msg, 1);
	}
	while (!list_empty(&quit_list)) {
		msg = list_pop_front(&quit_list, AMessage, entry);
		msg->done(msg, -1);
	}
	return (msg != NULL);
}
