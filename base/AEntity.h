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
	list_node<AComponent> _entity_node;
	bool  valid() { return !_entity_node.empty(); }

	void  init(AObject *o, const char *n, int i = 0) {
		_self = o; _name = n; _index = i;
		_entity = NULL; _entity_node.init(this);
	}
	void  exit(); //{ release_f(_entity, NULL, _entity->_self->release2()); }
};

struct AEntity {
	AObject        *_self;
	AEntityManager *_manager;
	struct rb_node  _manager_node;
	list_node<AComponent> _com_list;
	int                   _com_count;

	void  init(AObject *o) {
		_self = o; _manager = NULL;
		RB_CLEAR_NODE(&_manager_node);
		_com_list.init(NULL);
		_com_count = 0;
	}
	void  exit() {
		assert(_com_list.empty());
		assert(RB_EMPTY_NODE(&_manager_node));
	}

	bool  _push(AComponent *c) {
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
	bool  _pop(AComponent *c) {
		bool valid = ((c->_entity == this) && !c->_entity_node.empty());
		if (valid) {
			//c->_self->release2();
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
};

static inline int
AEntityCmp(AEntity *key, AEntity *data) {
	if (key == data) return 0;
	return (key < data) ? -1 : 1;
}
rb_tree_declare(AEntity, AEntity*)

struct AEntityManager {
	struct rb_root _entity_map;
	int     _entity_count;

	void  init() {
		INIT_RB_ROOT(&_entity_map);
		_entity_count = 0;
	}
	bool  _push(AEntity *e) {
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
	bool  _valid(AEntity *e) {
		return ((e->_manager == this) && !RB_EMPTY_NODE(&e->_manager_node));
	}
	bool  _pop(AEntity *e) {
		bool valid = _valid(e);
		if (valid) {
			rb_erase(&e->_manager_node, &_entity_map);
			RB_CLEAR_NODE(&e->_manager_node);
			--_entity_count;
		} else {
			assert(0);
		}
		//if (valid) e->_self->release2();
		return valid;
	}
	AEntity* _upper(AEntity *cur) {
		if (RB_EMPTY_ROOT(&_entity_map)) return NULL;
		else if (cur == NULL)            return rb_first_entry(&_entity_map, AEntity, _manager_node);
		else                             return rb_upper_AEntity(&_entity_map, cur);
	}
	AEntity* _next(AEntity *cur) {
		assert(_valid(cur));
		struct rb_node *node = rb_next(&cur->_manager_node);
		return (node ? rb_entry(node, AEntity, _manager_node) : NULL);
	}
	int   _next_each(AEntity *cur, int(*func)(AEntity *e, void *p), void *p) {
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
	int   _next_each_com(AEntity *cur, const char *com_name, int(*func)(AComponent *c, void *p), void *p, int com_index = -1) {
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
};

#ifndef _AMODULE_H_
inline void AComponent::exit() {
}
#else
inline void AComponent::exit() {
	release_f(_entity, NULL, _entity->_self->release2());
}

template <typename lock_t>
struct EMTmpl {
	EM     *manager;
	lock_t *locker;

	void init(EM *em, lock_t *l) {
		manager = em; locker = l;
	}
	// Enity
	bool push(AEntity *e) {
		locker->lock();
		bool valid = manager->_push(e);
		if (valid) e->_self->addref();
		locker->unlock();
		return valid;
	}
	bool pop(AEntity *e) {
		locker->lock();
		bool valid = manager->_pop(e);
		locker->unlock();
		if (valid) e->_self->release2();
		return valid;
	}
	AEntity* next(AEntity *cur) {
		locker->lock();
		AEntity *e = manager->_upper(cur);
		if (e != NULL) e->_self->addref();
		locker->unlock();
		return e;
	}
	int  next_each(AEntity *cur, int(*func)(AEntity *e, void *p), void *p) {
		locker->lock();
		int result = manager->_next_each(cur, func, p);
		locker->unlock();
		return result;
	}
	// Component
	bool append(AEntity *e, AComponent *c) {
		locker->lock();
		bool valid = (manager->_valid(e) && e->_push(c));
		if (valid) {
			e->_self->addref();
			c->_self->addref();
		}
		locker->unlock();
		return valid;
	}
	bool remove(AEntity *e, AComponent *c) {
		locker->lock();
		bool valid = (manager->_valid(e) && e->_pop(c));
		locker->unlock();
		if (valid) c->_self->release2();
		return valid;
	}
	AComponent* get_com(AEntity *e, const char *com_name, int com_index = -1) {
		AComponent *c = NULL;
		locker->lock();
		if (manager->_valid(e)) c = e->_get(com_name, com_index);
		if (c != NULL) c->_self->addref();
		locker->unlock();
		return c;
	}
	AComponent* next_com(AEntity *cur, const char *com_name, int com_index = -1) {
		locker->lock();
		AComponent *c = manager->_next_com(cur, com_name, com_index);
		if (c != NULL) c->_self->addref();
		locker->unlock();
		return c;
	}
	int  next_each_com(AEntity *cur, const char *com_name, int(*func)(AComponent *c, void *p), void *p, int com_index = -1) {
		locker->lock();
		int result = manager->_next_each_com(cur, com_name, func, p, com_index);
		locker->unlock();
		return result;
	}
};
#endif

#endif
