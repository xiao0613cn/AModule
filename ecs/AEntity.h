#ifndef _AENTITY_H_
#define _AENTITY_H_
#include "../base/AModule_API.h"

typedef struct AEntity AEntity;
typedef struct AComponent AComponent;
typedef struct AEntityManager AEntityManager;

struct AComponent {
	const char *_name;
	int         _index : 16;
	unsigned    _dynmng : 1;
	AEntity    *_entity;
	list_head   _entry;

	void init(const char *n, int i = 0) {
		_name = n; _index = i; _dynmng = 0;
		_entity = NULL; _entry.init();
	}
	template <typename TComponent>
	TComponent* other(TComponent **c, int com_index = -1) {
		return _entity->get(c, com_index);
	}
};

struct AEntity : public AObject {
	AEntityManager *_manager;
	struct rb_node  _map_node;
	list_head       _com_list;
	int             _com_count;

	void init();
	void exit();
	bool valid();

	bool push(AComponent *c);
	bool pop(AComponent *c);
	AComponent* get(const char *com_name, int com_index);

	template <typename TComponent> void        init_push(TComponent *c, int i = 0);
	template <typename TComponent> void        pop_exit(TComponent *c);
	template <typename TComponent> TComponent* get(TComponent **c, int com_index = -1);
};

struct AEntityManagerMethod {
	int         (*_push)(AEntityManager *em, AEntity *e); // include e->addref()
	int         (*_pop)(AEntityManager *em, AEntity *e);  // include e->release()
	AEntity*    (*_find)(AEntityManager *em, void *key);
	AEntity*    (*_upper)(AEntityManager *em, void *key);
	AEntity*    (*_next)(AEntityManager *em, AEntity *cur);
	int         (*_upper_each)(AEntityManager *em, void *key, int(*func)(AEntity*,void*), void *p);
	void        (*_clear)(AEntityManager *em, BOOL iocom_shutdown/*=TRUE*/);

	AComponent* (*_upper_com)(AEntityManager *em, void *key, const char *com_name, int com_index);
	AComponent* (*_next_com)(AEntityManager *em, AEntity *cur, const char *com_name, int com_index);
	int         (*_upper_each_com)(AEntityManager *em, void *key, const char *com_name,
	                               int(*func)(AComponent*,void*), void *p, int com_index);
	AComponent* (*_add_com)(AEntityManager *em, AEntity *e, AModule *com_module, AOption *com_opts);
	void        (*_del_com)(AEntityManager *em, AEntity *e, AComponent *c);
};

struct AEntityManager : public AEntityManagerMethod {
	static const char* name() { return "AEntityManagerDefaultModule"; }
	static AEntityManager* get() { return AModule::singleton_data<AEntityManager>(); }

	struct rb_root   _entity_map;
	int              _entity_count;
	pthread_mutex_t  _mutex;
	void lock() { pthread_mutex_lock(&_mutex); }
	void unlock() { pthread_mutex_unlock(&_mutex); }

	// for ASystemManager execute
	struct ASystemManager *_sysmng;
	AThread         *_thr_entities;
	AOperator        _asop_entities; // multiple operator execute
	DWORD            _tick_entities;
	AEntity         *_last_entity;

	void init(AEntityManagerMethod *m) {
		if (m != this)
			*(AEntityManagerMethod*)this = *m;

		INIT_RB_ROOT(&_entity_map);
		_entity_count = 0;
		pthread_mutex_init(&_mutex, NULL);
		_sysmng = NULL;
		_thr_entities = NULL;
		_asop_entities.timer();
		_tick_entities = 1000;
		_last_entity = NULL;
	}
	void exit() {
		assert(RB_EMPTY_ROOT(&_entity_map));
		pthread_mutex_destroy(&_mutex);
	}
	// helper template function
	template <typename TComponent>
	TComponent* upper_com(TComponent **c, void *key, int com_index = -1) {
		return *c = (TComponent*)_upper_com(this, key, TComponent::name(), com_index);
	}
	template <typename TComponent>
	TComponent* next_com(TComponent *c, int com_index = -1) {
		return (TComponent*)_next_com(this, c->_entity, TComponent::name(), com_index);
	}
	template <typename TComponent>
	TComponent* add_com(AEntity *e, AOption *com_opts) {
		return (TComponent*)_add_com(this, e, (AModule*)TComponent::get(), com_opts);
	}
	template <typename TComponent>
	TComponent* verify_com(AEntity *e, AOption *com_opts) {
		TComponent *c; if (e->get<TComponent>(&c) == NULL) c = add_com<TComponent>(e, com_opts);
		return c;
	}
};

//////////////////////////////////////////////////////////////////////////
// inline function implement
inline void AEntity::init() {
	_manager = NULL; RB_CLEAR_NODE(&_map_node);
	_com_list.init(); _com_count = 0;
}

inline void AEntity::exit() {
	while (!_com_list.empty()) {
		AComponent *c = list_pop_front(&_com_list, AComponent, _entry);
		if (c->_dynmng) _manager->_del_com(_manager, this, c);
		else            assert(0);
	}
	assert(RB_EMPTY_NODE(&_map_node));
}

inline bool AEntity::valid() {
	return !RB_EMPTY_NODE(&_map_node);
}

inline bool AEntity::push(AComponent *c) {
	bool valid = ((c->_entity == NULL) && c->_entry.empty());
	if (valid) {
		c->_entity = this;
		_com_list.push_back(&c->_entry);
		++_com_count;
	} else {
		assert(0);
	}
	return valid;
}
template <typename TComponent>
inline void AEntity::init_push(TComponent *c, int i) {
	c->init(TComponent::name(), i);
	c->init2();
	push(c);
}

inline bool AEntity::pop(AComponent *c) {
	bool valid = ((c->_entity == this) && !c->_entry.empty());
	if (valid) {
		c->_entry.leave();
		--_com_count;
	} else {
		assert(0);
	}
	return valid;
}

template <typename TComponent>
inline void AEntity::pop_exit(TComponent *c) {
	pop(c);
	c->exit2();
}

inline AComponent* AEntity::get(const char *com_name, int com_index = -1) {
	list_for_each2(c, &_com_list, AComponent, _entry) {
		if ((strcasecmp(c->_name, com_name) == 0)
		 && (com_index == -1 || com_index == c->_index))
			return c;
	}
	return NULL;
}

template <typename TComponent>
inline TComponent* AEntity::get(TComponent **c, int com_index) {
	return *c = (TComponent*)get(TComponent::name(), com_index);
}


#endif
