/**
 * \file Constraint.cc
 * \brief Implementation of the Constraint and Constraint_array classes
 * \author Julien Henry
 */
#include "Constraint.h"
#include "Expr.h"
#include "Debug.h"

////////////////////////////////////////////////
// Constraint
////////////////////////////////////////////////

Constraint::Constraint(ap_constyp_t constyp, Expr * expr, ap_scalar_t* scalar) {
	ap_texpr1_t * exp = ap_texpr1_copy(expr->getExpr());
	ap_cons = ap_tcons1_make(
			constyp,
			exp,
			scalar);
	// We have a memory leak if we do not free the expression. Indeed,
	// ap_texpr1_copy returns a pointer on an allocated structure...
	free(exp);
}
		
Constraint::~Constraint() {
	ap_tcons1_clear(&ap_cons);
}

void Constraint::print() {
	// TODO
}

ap_tcons1_t * Constraint::get_ap_tcons1() {
	return &ap_cons;
}

////////////////////////////////////////////////
// Constraint_array
////////////////////////////////////////////////

Constraint_array::Constraint_array() {
	ap_array_ready = false;
}

Constraint_array::Constraint_array(Constraint * c) {
	constraints.push_back(c);
	ap_array_ready = false;

}

Constraint_array::~Constraint_array() {
	std::vector<Constraint*>::iterator it = constraints.begin(), et = constraints.end();
	for (; it != et; it++) {
		delete (*it);
	}
	if (ap_array_ready)
		ap_tcons1_array_clear(&ap_array);
}

void Constraint_array::add_constraint(Constraint * cons) {
	if (ap_array_ready)
		ap_tcons1_array_clear(&ap_array);
	constraints.push_back(cons);
	ap_array_ready = false;
}

ap_environment_t * Constraint_array::getEnv() {
	return constraints[0]->get_ap_tcons1()->env;
}

ap_tcons1_array_t * Constraint_array::to_tcons1_array() {
	if (ap_array_ready)
		return &ap_array;
	// we have to create it
	if (constraints.empty()) {
		Environment env;
		ap_array = ap_tcons1_array_make(env.getEnv(),0);
	} else {
		// we suppose every constraint has the same environment
		Environment env(constraints[0]);
		ap_array = ap_tcons1_array_make(env.getEnv(),constraints.size());
		std::vector<Constraint*>::iterator it = constraints.begin(), et = constraints.end();
		int k = 0;
		for (; it != et; it++,k++) {
			ap_tcons1_t c = ap_tcons1_copy((*it)->get_ap_tcons1());
			ap_tcons1_array_set(&ap_array,k,&c);	
			//ap_tcons1_array_set(ap_array,k,(*it)->get_ap_tcons1());	
		}
	}
	ap_array_ready = true;
	return &ap_array;
}

ap_lincons1_array_t * Constraint_array::to_lincons1_array() {
	// TODO
	return NULL;
}

void Constraint_array::print() {
	// TODO
}
