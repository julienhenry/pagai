#ifndef _ABSTRACT_H
#define _ABSTRACT_H

#include <vector>

#include "ap_global1.h"

class Node;

/// Base class of abstract domains
class Abstract {

	public:
		ap_manager_t * man;
	public:
		ap_abstract1_t * main;
		ap_abstract1_t * pilot;

	protected:
		virtual void clear_all() = 0;

	public:

		virtual ~Abstract() {};

		/// set_top - the abstract domain is set to top
		virtual void set_top(ap_environment_t * env) = 0;

		/// set_top - the abstract domain is set to bottom
		virtual void set_bottom(ap_environment_t * env) = 0;

		/// change_environment - change the environment of the abstract value
		virtual void change_environment(ap_environment_t * env) = 0;

		/// compare the abstract value with another one
		/// return 0 in equal
		/// return 1 in case of this <= d
		/// return -1 in case of d <= this
		/// return -2 if not comparable
		int compare(Abstract * d);

		/// is_leq - return true iff this <= d
		virtual bool is_leq(Abstract * d);

		virtual bool is_eq(Abstract * d);

		/// is_bottom - return true iff the abstract value is at bottom
		virtual bool is_bottom() = 0;

		/// widening - applies the widening operator, according to its
		/// definition in the domain.
		virtual void widening(Abstract * X) = 0;
		virtual void widening_threshold(Abstract * X, ap_lincons1_array_t* cons) = 0;

		/// meet_tcons_array - intersect the abstract domain with an array of
		/// constraints
		virtual void meet_tcons_array(ap_tcons1_array_t* tcons) = 0;

		/// canonicalize - canonicalize the apron representation of the abstract 
		//domain
		virtual void canonicalize() = 0;

		/// assign_texpr_array - assign a expression to a set of variables
		virtual void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest) = 0;
		
		/// join_array - the abstract value becomes the join of a set of
		/// abstract values
		virtual void join_array(ap_environment_t * env, std::vector<Abstract*> X_pred) = 0;

		virtual void join_array_dpUcm(ap_environment_t *env, Abstract* n) = 0;
		
		/// to_tcons_array - convert the abstract value to a conjunction of
		// tree constraints
		virtual ap_tcons1_array_t to_tcons_array() = 0;

		/// to_lincons_array - convert the abstract value to a conjunction of
		// linear constraints
		virtual ap_lincons1_array_t to_lincons_array() = 0;

		/// print - print the abstract domain on standard output
		virtual void print(bool only_main = false) = 0;
};
#endif
