#ifndef _AENTITY_H_
#define _AENTITY_H_

struct AObject;
typedef struct AEntity AEntity;
typedef struct AComponent AComponent;
typedef struct AEntityManager AEntityManager, EM;

struct AComponent {
	AObject    *_self;
	const char *_name;
	long        _index;
	AEntity    *_entity;
	list_head   _entity_node;

	void init(AObject *o, const char *n, int i = 0) {
		_self = o; _name = n; _index = i;
		_entity = NULL; _entity_node.init();
	}
	bool valid();
};

struct AEntity {
	AObject        *_self;
	AEntityManager *_manager;
	struct rb_node  _manager_node;
	list_head _com_list;
	int       _com_count;

	void init(AObject *o) {
		_self = o; _manager = NULL;
		RB_CLEAR_NODE(&_manager_node);
		_com_list.init();
		_com_count = 0;
	}
	void exit() {
		assert(_com_list.empty());
		assert(RB_EMPTY_NODE(&_manager_node));
	}
	bool valid() { return !RB_EMPTY_NODE(&_manager_node); }

	bool _push(AComponent *c) {
		bool valid = ((c->_entity == NULL) && c->_entity_node.empty());
		if (valid) {
			//c->_self->addref();
			//this->_self->addref();
			c->_entity = this;
			_com_list.push_back(&c->_entity_node);
			++_com_count;
		} else {
			assert(0);
		}
		return valid;
	}
	bool _pop(AComponent *c) {
		bool valid = ((c->_entity == this) && !c->_entity_node.empty());
		if (valid) {
			//c->_self->release();
			c->_entity_node.leave();
			--_com_count;
		} else {
			assert(0);
		}
		return valid;
	}
	AComponent* _get(const char *com_name, int com_index = -1) {
		AComponent *c;
		list_for_each_entry(c, &_com_list, AComponent, _entity_node) {
			if ((strcasecmp(c->_name, com_name) == 0)
			 && (com_index == -1 || com_index == c->_index)) {
				//c->_self->addref();
				return c;
			}
		}
		return NULL;
	}
	template <typename TComponent>
	TComponent* _get(int com_index = -1) {
		return (TComponent*)_get(TComponent::name(), com_index);
	}
};

static inline int
AEntityCmp(AEntity *key, AEntity *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
rb_tree_declare(AEntity, AEntity*)

struct AEntityManager {
	struct rb_root _entity_map;
	int            _entity_count;

	void init() {
		INIT_RB_ROOT(&_entity_map);
		_entity_count = 0;
	}
	bool _push(AEntity *e) {
		bool valid = ((e->_manager == NULL) && RB_EMPTY_NODE(&e->_manager_node));
		if (valid)
			valid = (rb_insert_AEntity(&_entity_map, e, e) == NULL);
		if (valid) {
			//e->_self->addref();
			e->_manager = this;
			++_entity_count;
		} else {
			assert(0);
		}
		return valid;
	}
	bool _pop(AEntity *e) {
		bool valid = e->valid();
		if (valid) {
			rb_erase(&e->_manager_node, &_entity_map);
			RB_CLEAR_NODE(&e->_manager_node);
			--_entity_count;
		} else {
			assert(0);
		}
		//if (valid) e->_self->release();
		return valid;
	}
	AEntity* _upper(AEntity *cur) {
		if (RB_EMPTY_ROOT(&_entity_map)) return NULL;
		else if (cur == NULL)            return rb_first_entry(&_entity_map, AEntity, _manager_node);
		else                             return rb_upper_AEntity(&_entity_map, cur);
	}
	AEntity* _next(AEntity *cur) {
		assert(cur->valid());
		struct rb_node *node = rb_next(&cur->_manager_node);
		return (node ? rb_entry(node, AEntity, _manager_node) : NULL);
	}
	int  _next_each(AEntity *cur, int(*func)(AEntity *e, void *p), void *p) {
		int result = 0;
		cur = _upper(cur);
		while (cur != NULL) {
			result = func(cur, p);
			if (result != 0) break;
			cur = _next(cur);
		}
		return result;
	}
	AComponent* _next_com(AEntity *cur, const char *com_name, int com_index = -1) {
		cur = _upper(cur);
		while (cur != NULL) {
			AComponent *c = cur->_get(com_name, com_index);
			if (c != NULL) return c;
			cur = _next(cur);
		}
		return NULL;
	}
	template <typename TComponent>
	TComponent* _next_com(AEntity *cur, int com_index = -1 ) {
		return _next_com(cur, TComponent::name(), com_index);
	}
	int  _next_each_com(AEntity *cur, const char *com_name, int(*func)(AComponent *c, void *p), void *p, int com_index = -1) {
		int result = 0;
		cur = _upper(cur);
		while (cur != NULL) {
			AComponent *c = cur->_get(com_name, com_index);
			if (c != NULL) {
				result = func(c, p);
				if (result != 0) break;
			}
			cur = _next(cur);
		}
		return result;
	}
	template <typename TComponent>
	int _next_each_com(AEntity *cur, int(*func)(TComponent*,void*), void *p, int com_index = -1) {
		return _next_each_com(cur, TComponent::name(), (int(*)(AComponent*,void*))func, com_index);
	}
};

inline bool AComponent::valid() {
	return _entity->valid() && !_entity_node.empty();
}
#ifndef _AMODULE_H_
#else
// outside component of entity, has self refcount
struct AComponent2 : public AObject, public AComponent {
	void init(const char *n, int i = 0) {
		AComponent::init(this, n, i);
	}
	void exit() {
		assert(_entity_node.empty());
		release_f(_entity, NULL, _entity->_self->release());
	}
};

struct AEntity2 : public AObject, public AEntity {
	void  init() { AEntity::init(this); }
};

#if 0
struct AEntityManager2 : public AEntityManager {
	pthread_mutex_t _mutex;

	void init() {
		AEntityManager::init();
		pthread_mutex_init(&_mutex, NULL);
	}
	void exit() {
		assert(RB_EMPTY_ROOT(&_entity_map));
		assert(_entity_count == 0);
		pthread_mutex_destroy(&_mutex);
	}
	void entity_lock() { pthread_mutex_lock(&_mutex); }
	void entity_unlock() { pthread_mutex_unlock(&_mutex); }
	// Enity
	bool push(AEntity *e) {
		entity_lock();
		bool valid = _push(e);
		if (valid) e->_self->addref();
		entity_unlock();
		return valid;
	}
	bool pop(AEntity *e) {
		entity_lock();
		bool valid = _pop(e);
		entity_unlock();
		if (valid) e->_self->release();
		return valid;
	}
	AEntity* next(AEntity *cur) {
		entity_lock();
		AEntity *e = _upper(cur);
		if (e != NULL) e->_self->addref();
		entity_unlock();
		return e;
	}
	int  next_each(AEntity *cur, int(*func)(AEntity *e, void *p), void *p) {
		entity_lock();
		int result = _next_each(cur, func, p);
		entity_unlock();
		return result;
	}
	// Component
	bool append(AEntity *e, AComponent *c) {
		assert(e->_self != c->_self);
		entity_lock();
		bool valid = (e->valid() && e->_push(c));
		if (valid) {
			e->_self->addref();
			c->_self->addref();
		}
		entity_unlock();
		return valid;
	}
	bool remove(AEntity *e, AComponent *c) {
		assert(e->_self != c->_self);
		entity_lock();
		bool valid = (/*e->valid() && */e->_pop(c));
		entity_unlock();
		if (valid) c->_self->release();
		return valid;
	}
	template <typename TComponent>
	TComponent* get_com(AEntity *e, int com_index = -1) {
		TComponent *c = NULL;
		entity_lock();
		if (e->valid()) c = e->_get<TComponent>(com_index);
		if (c != NULL) c->_self->addref();
		entity_unlock();
		return c;
	}
	template <typename TComponent>
	TComponent* next_com(AEntity *cur, int com_index = -1) {
		entity_lock();
		TComponent *c = _next_com<TComponent>(cur, com_index);
		if (c != NULL) c->_self->addref();
		entity_unlock();
		return c;
	}
	template <typename TComponent>
	int  next_each_com(AEntity *cur, int(*func)(TComponent *c, void *p), void *p, int com_index = -1) {
		entity_lock();
		int result = _next_each_com<TComponent>(cur, func, p, com_index);
		entity_unlock();
		return result;
	}
};
#endif
#endif

#endif
