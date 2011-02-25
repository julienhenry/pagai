#include "ap_global1.h"

#include "Abstract.h"
#include "Node.h"

Abstract::Abstract(ap_manager_t* _man) {
	ap_environment_t * env = ap_environment_alloc_empty();
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	pilot = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	man = _man;
}

Abstract::~Abstract() {
	ap_abstract1_clear(man,main);
	ap_abstract1_clear(man,pilot);
}

bool Abstract::is_leq (Abstract *d) {
	if (ap_abstract1_is_eq(man,main,d->main)) {
		if (ap_abstract1_is_leq(man,pilot,d->pilot)) 
			return true; 
		else 
			return false;
	}
	if (ap_abstract1_is_leq(man,main,d->main))
		return true;
	return false;
}

void Abstract::widening(Node * n) {
		ap_abstract1_t Xmain_widening;
		ap_abstract1_t Xpilot_widening;

		if (is_leq(n->X)) {
			Xmain_widening = *n->X->main;
			Xpilot_widening = *n->X->pilot;
		} else if (ap_abstract1_is_leq(man,pilot,n->X->pilot)) {
			//Xmain_widening = *Xtemp.pilot;
			Xmain_widening = ap_abstract1_copy(man,pilot);
			Xpilot_widening = ap_abstract1_copy(man,pilot);
		} else {
			Xmain_widening = ap_abstract1_join(man,false,n->X->main,main);
			Xpilot_widening = ap_abstract1_widening(man,n->X->pilot,pilot);
			ap_abstract1_clear(man,pilot);
		}
		main = new ap_abstract1_t(Xmain_widening);
		pilot = new ap_abstract1_t(Xpilot_widening);
}
