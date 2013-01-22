/**
 * \file AbstractClassic.cc
 * \brief Implementation of the AbstractClassic class
 * \author Julien Henry
 */
#include "stdio.h"

#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Abstract.h"
#include "AbstractClassic.h"
#include "Node.h"
#include "Expr.h"
#include "apron.h"
#include "Analyzer.h"

AbstractClassic::AbstractClassic(ap_manager_t* _man, Environment * env) {
	main = new ap_abstract1_t(ap_abstract1_bottom(_man,env->getEnv()));
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

void AbstractClassic::set_top(Environment * env) {
	ap_abstract1_clear(man,main);
	*main = ap_abstract1_top(man,env->getEnv());
}

void AbstractClassic::set_bottom(Environment * env) {
	ap_abstract1_clear(man,main);
	*main = ap_abstract1_bottom(man,env->getEnv());
}

void AbstractClassic::change_environment(Environment * env) {
	if (!ap_environment_is_eq(env->getEnv(),main->env))
		*main = ap_abstract1_change_environment(man,true,main,env->getEnv(),false);
}

bool AbstractClassic::is_bottom() {
	return ap_abstract1_is_bottom(man,main);
}

bool AbstractClassic::is_top() {
	return ap_abstract1_is_top(man,main);
}

void AbstractClassic::widening(Abstract * X) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening(man,X->main,&Xmain);
	ap_abstract1_clear(man,&Xmain);

	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
}

void AbstractClassic::widening_threshold(Abstract * X, Constraint_array* cons) {
	ap_abstract1_t Xmain_widening;
	ap_abstract1_t Xmain;

	Xmain = ap_abstract1_join(man,false,X->main,main);
	Xmain_widening = ap_abstract1_widening_threshold(man,X->main,&Xmain, cons->to_lincons1_array());
	ap_abstract1_clear(man,&Xmain);

	ap_abstract1_clear(man,main);
	*main = Xmain_widening;
}

void AbstractClassic::meet_tcons_array(Constraint_array* tcons) {
	Environment main_env(this);
	Environment cons_env(tcons);
	Environment lcenv(Environment::common_environment(&main_env,&cons_env));

	*main = ap_abstract1_change_environment(man,true,main,lcenv.getEnv(),false);
	*main = ap_abstract1_meet_tcons_array(man,true,main,tcons->to_tcons1_array());
	canonicalize();
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
	canonicalize();
}

void AbstractClassic::join_array(Environment * env, std::vector<Abstract*> X_pred) {
	size_t size = X_pred.size();
	ap_abstract1_clear(man,main);

	ap_abstract1_t  Xmain[size];

	for (unsigned i=0; i < size; i++) {
		Xmain[i] = ap_abstract1_change_environment(man,false,X_pred[i]->main,env->getEnv(),false);
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
	canonicalize();
}

void AbstractClassic::join_array_dpUcm(Environment *env, Abstract* n) {
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
	*Out << *this;
}

void AbstractClassic::display(llvm::raw_ostream &stream, std::string * left) const {
#if 1
	ap_tcons1_array_t tcons_array = ap_abstract1_to_tcons_array(man,main);
	size_t size = ap_tcons1_array_size(&tcons_array);
	if (ap_abstract1_is_bottom(man,main)) {
		if (left != NULL) stream << *left;
		stream << "UNREACHABLE\n";
	} else if (size == 0) {
		if (left != NULL) stream << *left;
		stream << "TOP\n";
	} else {
		for (size_t k = 0; k < size; k++) {
			ap_tcons1_t cons = ap_tcons1_array_get(&tcons_array,k);
			if (left != NULL) stream << *left;
			stream << cons << "\n";
		}
	}
	ap_tcons1_array_clear(&tcons_array);

#else
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
		stream << c;
	fclose(tmp);
#endif
}
