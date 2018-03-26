#include "stdafx.h"
#include "../base/AModule_API.h"
#include "AEntity.h"

static inline int AEntityCmp(void *key, AEntity *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
rb_tree_define(AEntity, _map_node, void*, AEntityCmp)

static int EM_push(AEntityManager *em, AEntity *e)
{
	bool valid = ((e->_manager == NULL) && RB_EMPTY_NODE(&e->_map_node));
	if (valid)
		valid = (rb_insert_AEntity(&em->_entity_map, e, e) == NULL);
	if (valid) {
		e->addref();
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
		e->release();
	} else {
		assert(0);
	}
	return valid;
}

static AEntity* EM_find(AEntityManager *em, void *key)
{
	return rb_find_AEntity(&em->_entity_map, key);
}

static AEntity* EM_upper(AEntityManager *em, void *key)
{
	if (RB_EMPTY_ROOT(&em->_entity_map))
		return NULL;
	if (key == NULL)
		return rb_first_entry(&em->_entity_map, AEntity, _map_node);
	return rb_upper_AEntity(&em->_entity_map, key);
}

static AEntity* EM_next(AEntityManager *em, AEntity *cur)
{
	assert(cur->_manager == em);
	struct rb_node *node = rb_next(&cur->_map_node);
	return (node ? rb_entry(node, AEntity, _map_node) : NULL);
}

static AComponent* EM_upper_com(AEntityManager *em, void *key, const char *com_name, int com_index)
{
	AEntity *cur = EM_upper(em, key);
	while (cur != NULL) {
		AComponent *c = cur->get(com_name, com_index);
		if (c != NULL) return c;
		cur = EM_next(em, cur);
	}
	return NULL;
}

static AComponent* EM_next_com(AEntityManager *em, AEntity *cur, const char *com_name, int com_index)
{
	assert(cur->_manager == em);
	struct rb_node *node;
	while ((node = rb_next(&cur->_map_node)) != NULL) {
		cur = rb_entry(node, AEntity, _map_node);
		AComponent *c = cur->get(com_name, com_index);
		if (c != NULL) return c;
	}
	return NULL;
}

static int EM_upper_each(AEntityManager *em, void *key, int(*func)(AEntity*,void*), void *p)
{
	int result = 0;
	AEntity *cur = EM_upper(em, key);
	while (cur != NULL) {
		result = func(cur, p);
		if (result != 0) break;
		cur = EM_next(em, cur);
	}
	return result;
}

static int EM_upper_each_com(AEntityManager *em, void *key, const char *com_name,
                             int(*func)(AComponent*,void*), void *p, int com_index)
{
	int result = 0;
	AEntity *cur = EM_upper(em, key);
	while (cur != NULL) {
		AComponent *c = cur->get(com_name, com_index);
		if (c != NULL) {
			result = func(c, p);
			if (result != 0) break;
		}
		cur = EM_next(em, cur);
	}
	return result;
}

static void EM_clear(AEntityManager *em)
{
	AEntity *cur = EM_upper(em, NULL);
	while (cur != NULL) {
		AEntity *next = EM_next(em, cur);
		EM_pop(em, cur);
		cur = next;
	}
}

struct EMComHead {
	AEntityManager *_manager;
	AEntity *_entity;
	AModule *_module;
};

static AComponent* EM_add_com(AEntityManager *em, AEntity *e, AModule *com_module)
{
	EMComHead *h = (EMComHead*)malloc(sizeof(EMComHead) + com_module->object_size);
	if (h == NULL)
		return NULL;

	h->_manager = em;
	h->_entity = e;
	h->_module = com_module;

	AComponent *c = (AComponent*)(h + 1);
	int result = com_module->create((AObject**)&c, e, NULL);
	if (result < 0) {
		em->_del_com(em, e, c);
		c = NULL;
	} else {
		c->init(com_module->module_name);
		c->_dynmng = 1;
		e->push(c);
	}
	return c;
}

static void EM_del_com(AEntityManager *em, AEntity *e, AComponent *c)
{
	EMComHead *h = (EMComHead*)((char*)c - sizeof(EMComHead));
	assert(c->_dynmng && (h->_manager == em) && (h->_entity == e));

	h->_module->release((AObject*)c);
	free(h);
}

extern struct EM_m {
	AModule module;
	union {
		AEntityManagerMethod method;
		AEntityManager manager;
	};
} EM_default;

static int EM_init(AOption *global_option, AOption *module_option, BOOL first)
{
	if (first) {
		EM_default.manager.init(&EM_default.method);
	}
	return 1;
}

static void EM_exit(int inited)
{
	if (inited > 0) {
		EM_default.manager.exit();
	}
}

static EM_m EM_default = { {
	AEntityManager::name(),
	AEntityManager::name(),
	0, &EM_init, &EM_exit,
}, {
	&EM_push,
	&EM_pop,
	&EM_find,
	&EM_upper,
	&EM_next,
	&EM_upper_each,
	&EM_clear,

	&EM_upper_com,
	&EM_next_com,
	&EM_upper_each_com,
	&EM_add_com,
	&EM_del_com,
} };
static int reg_mng = AModuleRegister(&EM_default.module);
