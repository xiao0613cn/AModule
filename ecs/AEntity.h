#ifndef _AENTITY_H_
#define _AENTITY_H_
#include "../base/AModule_API.h"

typedef struct AEntity AEntity;
typedef struct AComponent AComponent;
typedef struct AEntityManager AEntityManager;

struct AComponent {
	const char *_name;
	int         _index;
	AObject    *_object;
	list_head   _entry;

	void init(AObject *o, const char *n, int i = 0) {
		_name = n; _index = i;
		_object = o; _entry.init();
	}
	bool valid() {
		return !_entry.empty();
	}
	template <typename TComponent>
	TComponent* _other(TComponent **c, int com_index = -1) {
		return ((AEntity*)_object)->_get(c, com_index);
	}
};

struct AEntity : public AObject {
	AEntityManager *_manager;
	struct rb_node  _map_node;
	list_head       _com_list;
	int             _com_count;

	void init() {
		_manager = NULL;
		RB_CLEAR_NODE(&_map_node);
		_com_list.init();
		_com_count = 0;
	}
	void exit() {
		while (!_com_list.empty()) {
			AComponent *c = list_pop_front(&_com_list, AComponent, _entry);
			assert(c->_object != this);
			c->_object->release();
		}
		assert(RB_EMPTY_NODE(&_map_node));
	}
	bool valid() { return !RB_EMPTY_NODE(&_map_node); }

	bool _push(AComponent *c) {
		bool valid = c->_entry.empty();
		if (valid) {
			//c->_self->addref();
			//this->_self->addref();
			_com_list.push_back(&c->_entry);
			++_com_count;
		} else {
			assert(0);
		}
		return valid;
	}
	template <typename TComponent>
	void _init_push(TComponent *c, int i = 0) {
		c->init(this, TComponent::name(), i);
		c->init2();
		_push(c);
	}
	bool _pop(AComponent *c) {
		bool valid = !c->_entry.empty();
		if (valid) {
			//c->_self->release();
			c->_entry.leave();
			--_com_count;
		} else {
			assert(0);
		}
		return valid;
	}
	template <typename TComponent>
	void _pop_exit(TComponent *c) {
		_pop(c);
		c->exit2();
	}
	AComponent* _get(const char *com_name, int com_index = -1) {
		list_for_each2(c, &_com_list, AComponent, _entry) {
			if ((strcasecmp(c->_name, com_name) == 0)
			 && (com_index == -1 || com_index == c->_index)) {
				//c->_self->addref();
				return c;
			}
		}
		return NULL;
	}
	template <typename TComponent>
	TComponent* _get(TComponent **c, int com_index = -1) {
		return *c = (TComponent*)_get(TComponent::name(), com_index);
	}
};

struct AEntityManagerMethod {
	int         (*_push)(AEntityManager *em, AEntity *e);
	int         (*_pop)(AEntityManager *em, AEntity *e);
	AEntity*    (*_find)(AEntityManager *em, void *key);
	AEntity*    (*_upper)(AEntityManager *em, AEntity *cur);
	AEntity*    (*_next)(AEntityManager *em, AEntity *cur);
	AComponent* (*_upper_com)(AEntityManager *em, AEntity *cur, const char *com_name, int com_index);
	int         (*_upper_each)(AEntityManager *em, AEntity *cur, int(*func)(AEntity*,void*), void *p);
	int         (*_upper_each_com)(AEntityManager *em, AEntity *cur, const char *com_name,
	                               int(*func)(AComponent*,void*), void *p, int com_index);
};

struct AEntityManager : public AEntityManagerMethod {
	static const char* name() { return "AEntityManagerDefaultModule"; }
	static AEntityManager* get() { return AModule::singleton_data<AEntityManager>(); }

	struct rb_root   _entity_map;
	int              _entity_count;
	pthread_mutex_t  _mutex;

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
	void lock() { pthread_mutex_lock(&_mutex); }
	void unlock() { pthread_mutex_unlock(&_mutex); }

	template <typename TComponent>
	TComponent* _next_com(AEntity *cur, int com_index = -1 ) {
		return _upper_com(this, cur, TComponent::name(), com_index);
	}
	template <typename TComponent>
	int _next_each_com(AEntity *cur, int(*func)(TComponent*,void*), void *p, int com_index = -1) {
		return _upper_each_com(this, cur, TComponent::name(), (int(*)(AComponent*,void*))func, com_index);
	}
};

#if 0
// outside component of entity, has self refcount
struct AComponent2 : public AObject, public AComponent {
	void init(const char *n, int i = 0) {
		AComponent::init(this, n, i);
	}
	void exit() {
		assert(_entry.empty());
		reset_nif(_entity, NULL, _entity->_self->release());
	}
};
#endif

#endif
