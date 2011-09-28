#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "AbstractMan.h"
#include "AbstractDisj.h"
#include "Node.h"
#include "Expr.h"
#include "Analyzer.h"

AbstractDisj::AbstractDisj(ap_manager_t* _man, ap_environment_t * env) {
	man_disj = new AbstractManClassic();
	disj.push_back(man_disj->NewAbstract(_man,env));
	main = disj[0]->main;
	pilot = NULL;
	man = _man;
}

AbstractDisj::AbstractDisj(ap_manager_t* _man, ap_environment_t * env, int max_index) {
	man_disj = new AbstractManClassic();
	for (int i = 0; i <= max_index; i++)
		disj.push_back(man_disj->NewAbstract(_man,env));
	main = disj[0]->main;
	pilot = NULL;
	man = _man;
}

AbstractDisj::AbstractDisj(Abstract* A) {
	man_disj = new AbstractManClassic();
	man = A->man;
	disj.clear();
	if (AbstractDisj * A_dis = dynamic_cast<AbstractDisj*>(A)) {
		std::vector<Abstract*>::iterator it = A_dis->disj.begin(), et = A_dis->disj.end();
		for (; it != et; it++) {
			disj.push_back(man_disj->NewAbstract(*it));
		}
		main = disj[0]->main;
	} else {
		*Out << "ERROR when trying to create a disjunctive invariant\n";
		// ERROR
		main = NULL;
	}
	pilot = NULL;
}

void AbstractDisj::clear_all() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		delete *it;
	}
	disj.clear();
	main = NULL;
}

AbstractDisj::~AbstractDisj() {
	delete man_disj;
	clear_all();
}

/// set_top - sets the abstract to top on the environment env
void AbstractDisj::set_top(ap_environment_t * env) {
	set_top(env,0);
}

void AbstractDisj::set_top(ap_environment_t * env, int index) {
	int i = 0;
	// every disjunct is at bottom except the one of index 'index'
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++, i++) {
		if (i == index)
			(*it)->set_top(env);
		else
			(*it)->set_bottom(env);
	}
}

/// set_bottom - sets the abstract to bottom on the environment env
void AbstractDisj::set_bottom(ap_environment_t * env) {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->set_bottom(env);
	}
}

void AbstractDisj::set_bottom(ap_environment_t * env, int index) {
	disj[index]->set_bottom(env);
}


void AbstractDisj::change_environment(ap_environment_t * env) {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->change_environment(env);
	}
}

void AbstractDisj::change_environment(ap_environment_t * env, int index) {
	disj[index]->change_environment(env);
}

// NOT CORRECT
bool AbstractDisj::is_leq (Abstract *d) {
	return ap_abstract1_is_leq(man,main,d->main);
}

bool AbstractDisj::is_leq (Abstract *d, int index) {
	return disj[index]->is_leq(d);
}

// NOT CORRECT
bool AbstractDisj::is_eq (Abstract *d) {
		return ap_abstract1_is_eq(man,main,d->main);
}

bool AbstractDisj::is_eq (Abstract *d, int index) {
	return disj[index]->is_eq(d);
}

bool AbstractDisj::is_bottom() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		if (!(*it)->is_bottom()) return false;
	}
	return true;
}

bool AbstractDisj::is_bottom(int index) {
	return disj[index]->is_bottom();
}

//NOT CORRECT
void AbstractDisj::widening(Abstract * X) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening(man,X->main,&Xmain);
	ap_abstract1_clear(man,&Xmain);
	
	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
}

void AbstractDisj::widening(Abstract * X, int index) {
	disj[index]->widening(X);
}

//NOT CORRECT
void AbstractDisj::widening_threshold(Abstract * X, ap_lincons1_array_t* cons) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening_threshold(man,X->main,&Xmain, cons);
	ap_abstract1_clear(man,&Xmain);
	
	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
}

void AbstractDisj::widening_threshold(Abstract * X, ap_lincons1_array_t* cons, int index) {
	disj[index]->widening_threshold(X,cons);
}

void AbstractDisj::meet_tcons_array(ap_tcons1_array_t* tcons) {

	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->meet_tcons_array(tcons);
	}
}

void AbstractDisj::meet_tcons_array(ap_tcons1_array_t* tcons, int index) {
	disj[index]->meet_tcons_array(tcons);
}

void AbstractDisj::canonicalize() {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->canonicalize();
	}
}

void AbstractDisj::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest
		) {
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++) {
		(*it)->assign_texpr_array(tvar,texpr,size,dest);
	}
}

void AbstractDisj::assign_texpr_array(
		ap_var_t* tvar, 
		ap_texpr1_t* texpr, 
		size_t size, 
		ap_abstract1_t* dest,
		int index
		) {
	disj[index]->assign_texpr_array(tvar,texpr,size,dest);
}

//NOT CORRECT
void AbstractDisj::join_array(ap_environment_t * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();
	ap_abstract1_clear(man,main);

	ap_abstract1_t  Xmain[size];
	
	for (unsigned i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env,false);
		delete X_pred[i];
	}
	
	if (size > 1) {
		*main = ap_abstract1_join_array(man,Xmain,size);	
		for (unsigned i=0; i < size; i++) {
			ap_abstract1_clear(man,&Xmain[i]);
		}
	} else {
		*main = Xmain[0];
	}
}

void AbstractDisj::join_array(ap_environment_t * env, std::vector<Abstract*> X_pred, int index) {
	disj[index]->join_array(env,X_pred);
}

//NOT CORRECT
void AbstractDisj::join_array_dpUcm(ap_environment_t *env, Abstract* n) {
	std::vector<Abstract*> v;
	v.push_back(n);
	v.push_back(new AbstractDisj(this));
	join_array(env,v);
}

void AbstractDisj::join_array_dpUcm(ap_environment_t *env, Abstract* n, int index) {
	disj[index]->join_array_dpUcm(env,n);
}

//NOT CORRECT
ap_tcons1_array_t AbstractDisj::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_tcons1_array_t AbstractDisj::to_tcons_array(int index) {
	return disj[index]->to_tcons_array();
}

//NOT CORRECT
ap_lincons1_array_t AbstractDisj::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

ap_lincons1_array_t AbstractDisj::to_lincons_array(int index) {
	return disj[index]->to_lincons_array();
}

void AbstractDisj::print(bool only_main) {
	int k = 0;
	std::vector<Abstract*>::iterator it = disj.begin(), et = disj.end();
	for (; it != et; it++, k++) {
		*Out << "Disjunct " << k << "\n";
		(*it)->print(only_main);
	}
}
