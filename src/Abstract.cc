#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "Node.h"
#include "Expr.h"
#include "Analyzer.h"

Abstract::Abstract(ap_manager_t* _man, ap_environment_t * env) {
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	man = _man;
}


Abstract::Abstract(Abstract* A) {
	man = A->man;
	main = new ap_abstract1_t(ap_abstract1_copy(man,A->main));
}

void Abstract::clear_all() {
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
}

/// set_bottom - sets the abstract to bottom on the environment env
void Abstract::set_bottom(ap_environment_t * env) {
		clear_all();
		main = new ap_abstract1_t(ap_abstract1_bottom(man,env));
}

void Abstract::change_environment(ap_environment_t * env) {
	if (!ap_environment_is_eq(env,main->env))
		*main = ap_abstract1_change_environment(man,true,main,env,false);
}

bool Abstract::is_leq (Abstract *d) {
		return ap_abstract1_is_leq(man,main,d->main);
}

bool Abstract::is_bottom() {
	return ap_abstract1_is_bottom(man,main);
}

/// widening - Compute the widening operation according to the Gopan & Reps
/// approach
void Abstract::widening(Node * n) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,n->X->main,main);
	Xmain_widening = ap_abstract1_widening(man,n->X->main,&Xmain);
	ap_abstract1_clear(man,&Xmain);
	
	*main = Xmain_widening;
}

void Abstract::widening_threshold(Node * n, ap_lincons1_array_t* cons) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,n->X->main,main);
	Xmain_widening = ap_abstract1_widening_threshold(man,n->X->main,&Xmain, cons);
	ap_abstract1_clear(man,&Xmain);
	
	*main = Xmain_widening;
}

void Abstract::meet_tcons_array(ap_tcons1_array_t* tcons) {

	ap_environment_t * lcenv = common_environment(
			main->env,
			ap_tcons1_array_envref(tcons));

	*main = ap_abstract1_change_environment(man,true,main,lcenv,false);
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons);

}

void Abstract::canonicalize() {
	ap_abstract1_canonicalize(man,main);
}

void Abstract::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	*main = ap_abstract1_assign_texpr_array(man,true,main,
			tvar,
			texpr,
			size,
			dest);
}

void Abstract::join_array(ap_environment_t * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();

	ap_abstract1_t  Xmain[size];
	
	for (unsigned i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env,false);
		delete X_pred[i];
	}
	
	ap_abstract1_clear(man,main);
	if (size > 1) {
		*main = ap_abstract1_join_array(man,Xmain,size);	
		for (unsigned i=0; i < size; i++) {
			ap_abstract1_clear(man,&Xmain[i]);
		}
	} else {
		*main = Xmain[0];
	}
}

ap_tcons1_array_t Abstract::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_lincons1_array_t Abstract::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

void Abstract::print(bool only_main) {

	FILE* tmp = tmpfile();

	ap_environment_fdump(tmp,main->env);
	ap_abstract1_fprint(tmp,man,main);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;

}
