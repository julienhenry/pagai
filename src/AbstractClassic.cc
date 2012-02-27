#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "AbstractClassic.h"
#include "Node.h"
#include "Expr.h"
#include "Analyzer.h"

AbstractClassic::AbstractClassic(ap_manager_t* _man, ap_environment_t * env) {
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env));
	pilot = NULL;
	man = _man;
}


AbstractClassic::AbstractClassic(Abstract* A) {
	man = A->man;
	main = new ap_abstract1_t(ap_abstract1_copy(man,A->main));
	pilot = NULL;
}

void AbstractClassic::clear_all() {
	ap_abstract1_clear(man,main);
	delete main;
}

AbstractClassic::~AbstractClassic() {
	clear_all();
}

/// set_top - sets the abstract to top on the environment env
void AbstractClassic::set_top(ap_environment_t * env) {
		ap_abstract1_clear(man,main);
		*main = ap_abstract1_top(man,env);
}

/// set_bottom - sets the abstract to bottom on the environment env
void AbstractClassic::set_bottom(ap_environment_t * env) {
		ap_abstract1_clear(man,main);
		*main = ap_abstract1_bottom(man,env);
}

void AbstractClassic::change_environment(ap_environment_t * env) {
	if (!ap_environment_is_eq(env,main->env))
		*main = ap_abstract1_change_environment(man,true,main,env,false);
}

//bool AbstractClassic::is_leq (Abstract *d) {
//		return ap_abstract1_is_leq(man,main,d->main);
//}
//
//bool AbstractClassic::is_eq (Abstract *d) {
//		return ap_abstract1_is_eq(man,main,d->main);
//}

bool AbstractClassic::is_bottom() {
	return ap_abstract1_is_bottom(man,main);
}

/// widening - Compute the widening operation according to the Gopan & Reps
/// approach
void AbstractClassic::widening(Abstract * X) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening(man,X->main,&Xmain);
	ap_abstract1_clear(man,&Xmain);
	
	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
}

void AbstractClassic::widening_threshold(Abstract * X, ap_lincons1_array_t* cons) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening_threshold(man,X->main,&Xmain, cons);
	ap_abstract1_clear(man,&Xmain);
	
	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
}

void AbstractClassic::meet_tcons_array(ap_tcons1_array_t* tcons) {

	ap_environment_t * lcenv = common_environment(
			main->env,
			ap_tcons1_array_envref(tcons));

	*main = ap_abstract1_change_environment(man,true,main,lcenv,false);
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons);

}

void AbstractClassic::canonicalize() {
	ap_abstract1_canonicalize(man,main);
}

void AbstractClassic::assign_texpr_array(
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

void AbstractClassic::join_array(ap_environment_t * env, std::vector<Abstract*> X_pred) {
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

void AbstractClassic::join_array_dpUcm(ap_environment_t *env, Abstract* n) {
	std::vector<Abstract*> v;
	v.push_back(n);
	v.push_back(new AbstractClassic(this));
	join_array(env,v);
}

void AbstractClassic::meet(Abstract* A) {
	ap_abstract1_t tmp = *main;
	*main = ap_abstract1_meet(man,false,main,A->main);
	ap_abstract1_clear(man,&tmp);
}

ap_tcons1_array_t AbstractClassic::to_tcons_array() {
	return ap_abstract1_to_tcons_array(man,main);
}

ap_lincons1_array_t AbstractClassic::to_lincons_array() {
	return ap_abstract1_to_lincons_array(man,main);
}

void AbstractClassic::print(bool only_main) {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR WHEN PRINTING ABSTRACT VALUE\n";
		return;
	}

	ap_environment_fdump(tmp,main->env);
	ap_abstract1_fprint(tmp,man,main);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}
