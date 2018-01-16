#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEntity.h"

static inline int AEntityCmp(AEntity *key, AEntity *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
rb_tree_define(AEntity, _map_node, AEntity*, AEntityCmp)

static int EM_push(AEntityManager *em, AEntity *e)
{
	bool valid = ((e->_manager == NULL) && RB_EMPTY_NODE(&e->_map_node));
	if (valid)
		valid = (rb_insert_AEntity(&em->_entity_map, e, e) == NULL);
	if (valid) {
		//e->_self->addref();
		e->_manager = em;
		++em->_entity_count;
	} else {
		assert(0);
	}
	return valid;
}

static int EM_pop(AEntityManager *em, AEntity *e)
{
	bool valid = ((e->_manager == em) && !RB_EMPTY_NODE(&e->_map_node));
	if (valid) {
		rb_erase(&e->_map_node, &em->_entity_map);
		RB_CLEAR_NODE(&e->_map_node);
		--em->_entity_count;
	} else {
		assert(0);
	}
	//if (valid) e->_self->release();
	return valid;
}

static AEntity* EM_find(AEntityManager *em, void *key)
{
	return rb_find_AEntity(&em->_entity_map, (AEntity*)key);
}

static AEntity* EM_upper(AEntityManager *em, AEntity *cur)
{
	if (RB_EMPTY_ROOT(&em->_entity_map))
		return NULL;
	if (cur == NULL)
		return rb_first_entry(&em->_entity_map, AEntity, _map_node);
	return rb_upper_AEntity(&em->_entity_map, cur);
}

static AEntity* EM_next(AEntityManager *em, AEntity *cur)
{
	assert(cur->_manager == em);
	struct rb_node *node = rb_next(&cur->_map_node);
	return (node ? rb_entry(node, AEntity, _map_node) : NULL);
}

static AComponent* EM_upper_com(AEntityManager *em, AEntity *cur, const char *com_name, int com_index)
{
	cur = EM_upper(em, cur);
	while (cur != NULL) {
		AComponent *c = cur->_get(com_name, com_index);
		if (c != NULL) return c;
		cur = EM_next(em, cur);
	}
	return NULL;
}

static int EM_upper_each(AEntityManager *em, AEntity *cur, int(*func)(AEntity*,void*), void *p)
{
	int result = 0;
	cur = EM_upper(em, cur);
	while (cur != NULL) {
		result = func(cur, p);
		if (result != 0) break;
		cur = EM_next(em, cur);
	}
	return result;
}

static int EM_upper_each_com(AEntityManager *em, AEntity *cur, const char *com_name,
                             int(*func)(AComponent*,void*), void *p, int com_index)
{
	int result = 0;
	cur = EM_upper(em, cur);
	while (cur != NULL) {
		AComponent *c = cur->_get(com_name, com_index);
		if (c != NULL) {
			result = func(c, p);
			if (result != 0) break;
		}
		cur = EM_next(em, cur);
	}
	return result;
}


AEntityManagerDefaultModule EntityMngModule = { {
	AEntityManagerDefaultModule::name(),
	AEntityManagerDefaultModule::name(),
	0, NULL, NULL,
}, {
	&EM_push,
	&EM_pop,
	&EM_find,
	&EM_upper,
	&EM_next,
	&EM_upper_com,
	&EM_upper_each,
	&EM_upper_each_com,
} };
static int reg_mng = AModuleRegister(&EntityMngModule.module);
