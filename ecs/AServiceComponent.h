#ifndef _AMODULE_ASERVICE_H_
#define _AMODULE_ASERVICE_H_

#include "AEntity.h"

struct AServiceComponent : public AComponent {
	static const char* name() { return "AServiceComponent"; }
	AMODULE_GET(struct AServiceComponentModule, name(), name());

	ASystemManager *_sysmng; // set by user
	AServiceComponent *_parent;
	struct list_head _children_list;
	struct list_head _brother_entry;

	unsigned int _save_option : 1;
	unsigned int _require_child : 1;
	unsigned int _post_start : 1;
	unsigned int _running : 1;

	AOption *_svc_option;
	AModule *_peer_module;
	int    (*start)(AServiceComponent *service, AOption *option);
	void   (*stop)(AServiceComponent *service);
	int    (*run)(AServiceComponent *service, AObject *peer);
	int    (*abort)(AServiceComponent *service, AObject *peer);

	void init2() {
		_sysmng = NULL; _parent = NULL;
		_children_list.init(); _brother_entry.init();
		_save_option = _require_child = _post_start = _running = 0;
		_svc_option = NULL; _peer_module = NULL;
		start = NULL; stop = NULL; run = NULL; abort = NULL;
	}
	void exit2() {
		reset_nif(_parent, NULL, _parent->_entity->release());
		if (_save_option)
			release_s(_svc_option);
	}

#define list_for_AService(svc, services) \
	list_for_each2(svc, &(services)->_children_list, AServiceComponent, _brother_entry)

	void _abort(AObject *peer) {
		if (abort) abort(this, peer);
		list_for_AService(svc, this) {
			svc->_abort(peer);
		}
	}
};

struct AServiceComponentModule {
	AModule module;
	AMODULE_GET(AServiceComponentModule, AServiceComponent::name(), AServiceComponent::name());

	int  (*start)(AServiceComponent *server, AOption *option, BOOL create_chains);
	void (*stop)(AServiceComponent *server, BOOL clean_chains);
	AServiceComponent* (*probe)(AServiceComponent *server, AObject *object, AMessage *msg);
	AServiceComponent* (*find)(AServiceComponent *server, const char *svc_name, BOOL find_in_chains);
};


#endif
