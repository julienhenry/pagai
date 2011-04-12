#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "Node.h"
#include "Expr.h"

Abstract::Abstract(ap_manager_t* _man, ap_environment_t * env) {
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	pilot = main;
	man = _man;
}


Abstract::Abstract(Abstract* A) {
	man = A->man;
	main = new ap_abstract1_t(ap_abstract1_copy(man,A->main));
	if (A->pilot == A->main)
		pilot = main;
	else
		pilot = new ap_abstract1_t(ap_abstract1_copy(man,A->pilot));
}

void Abstract::clear_all() {
	if (pilot != main) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
	}
	ap_abstract1_clear(man,main);
	delete main;
}

Abstract::~Abstract() {
	clear_all();
}

/// set_top - sets the abstract to top on the environment env
void Abstract::set_top(ap_environment_t * env) {
		clear_all();
		main = new ap_abstract1_t(ap_abstract1_top(man,env));
		pilot = main;
}

/// set_bottom - sets the abstract to bottom on the environment env
void Abstract::set_bottom(ap_environment_t * env) {
		clear_all();
		main = new ap_abstract1_t(ap_abstract1_bottom(man,env));
		pilot = main;
}

void Abstract::change_environment(ap_environment_t * env) {
	if (!ap_environment_is_eq(env,main->env))
		*main = ap_abstract1_change_environment(man,true,main,env,false);
	if (pilot != main && !ap_environment_is_eq(env,pilot->env))
		*pilot = ap_abstract1_change_environment(man,true,pilot,env,false);
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

bool Abstract::is_bottom() {
	return ap_abstract1_is_bottom(man,main);
}

/// widening - Compute the widening operation according to the Gopan & Reps
/// approach
void Abstract::widening(Node * n) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xpilot_widening;
	ap_abstract1_t Xpilot;
	ap_abstract1_t dpUcm;

	if (is_leq(n->X)) {
		Xmain_widening = ap_abstract1_copy(man,n->X->main);
		Xpilot_widening = ap_abstract1_copy(man,n->X->pilot);
	} else {
		dpUcm = ap_abstract1_join(man,false,n->X->main,pilot);
		if (ap_abstract1_is_leq(man,&dpUcm,n->X->pilot)) {
			Xmain_widening = ap_abstract1_copy(man,&dpUcm);
			Xpilot_widening = ap_abstract1_copy(man,&dpUcm);
		} else {
			Xmain_widening = ap_abstract1_join(man,false,n->X->main,main);
			// before widening, n->X->pilot has to be included in pilot
			Xpilot = ap_abstract1_join(man,false,n->X->pilot,&dpUcm);
			Xpilot_widening = ap_abstract1_widening(man,n->X->pilot,&Xpilot);
			ap_abstract1_clear(man,&Xpilot);
		}
	}

	if (pilot != main)
		ap_abstract1_clear(man,pilot);
	ap_abstract1_clear(man,main);
	
	*main = Xmain_widening;
	if (ap_abstract1_is_eq(man,&Xmain_widening,&Xpilot_widening)) {
		pilot = main;
		ap_abstract1_clear(man,&Xpilot_widening);
	} else {
		if (pilot != main)
			delete pilot;
		pilot = new ap_abstract1_t(Xpilot_widening);
	}
}

void Abstract::meet_tcons_array(ap_tcons1_array_t* tcons) {

	ap_environment_t * lcenv = common_environment(
			main->env,
			ap_tcons1_array_envref(tcons));

	if (pilot != main) {
		*pilot = ap_abstract1_change_environment(man,true,pilot,lcenv,false);
		*pilot = ap_abstract1_meet_tcons_array(man,true,pilot,tcons);
	}

	*main = ap_abstract1_change_environment(man,true,main,lcenv,false);
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons);

}

void Abstract::canonicalize() {
	ap_abstract1_canonicalize(man,main);
	if (pilot != main)
		ap_abstract1_canonicalize(man,pilot);
}

void Abstract::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	if (pilot != main)
		*pilot = ap_abstract1_assign_texpr_array(man,true,pilot,
				tvar,
				texpr,
				size,
				dest);

	*main = ap_abstract1_assign_texpr_array(man,true,main,
			tvar,
			texpr,
			size,
			dest);
}

void Abstract::join_array(ap_environment_t * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();

	ap_abstract1_t  Xmain[size];
	ap_abstract1_t  Xpilot[size];
	
	for (unsigned i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env,false);
		Xpilot[i] = ap_abstract1_change_environment(man,false,X_pred[i]->pilot,env,false);
		delete X_pred[i];
	}
	
	ap_abstract1_clear(man,main);
	if (pilot != main)
		ap_abstract1_clear(man,pilot);
	if (size > 1) {
		*main = ap_abstract1_join_array(man,Xmain,size);	
		pilot = new ap_abstract1_t(ap_abstract1_join_array(man,Xpilot,size));	
		for (unsigned i=0; i < size; i++) {
			ap_abstract1_clear(man,&Xmain[i]);
			ap_abstract1_clear(man,&Xpilot[i]);
		}
	} else {
		*main = Xmain[0];
		pilot = new ap_abstract1_t(Xpilot[0]);
	}
	
	if (ap_abstract1_is_eq(man,main,pilot)) {
		ap_abstract1_clear(man,pilot);
		delete pilot;
		pilot = main;
	}
}

ap_tcons1_array_t Abstract::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_lincons1_array_t Abstract::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

void Abstract::print(bool only_main) {
	if (!only_main)
		printf("MAIN VALUE:\n");
	ap_environment_fdump(stdout,main->env);
	ap_abstract1_fprint(stdout,man,main);

	if (!only_main) {
		printf("PILOT VALUE:\n");
		ap_environment_fdump(stdout,pilot->env);
		ap_abstract1_fprint(stdout,man,pilot);
	}
	fflush(stdout);
}
