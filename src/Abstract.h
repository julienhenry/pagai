#ifndef _ABSTRACT_H
#define _ABSTRACT_H

#include <vector>

#include "ap_global1.h"

class Node;

class Abstract {

	private:
		ap_manager_t * man;
	public:
		ap_abstract1_t * main;
		ap_abstract1_t * pilot;

	private:
		void clear_all();

	public:

		Abstract(ap_manager_t* _man, ap_environment_t * env);

		/// copy constructor : duplicates the abstract domain
		Abstract(Abstract* A);

		~Abstract();

		/// set_top - the abstract domain is set to top
		void set_top(ap_environment_t * env);

		/// set_top - the abstract domain is set to bottom
		void set_bottom(ap_environment_t * env);

		/// change_environment - change the environment of the abstract value
		void change_environment(ap_environment_t * env);

		/// is_leq - return true iff this <= d
		bool is_leq(Abstract * d);

		/// widening - applies the widening operator, according to its
		/// definition in the domain.
		void widening(Node * n);

		/// meet_tcons_array - intersect the abstract domain with an array of
		/// constraints
		void meet_tcons_array(ap_tcons1_array_t* tcons);

		/// canonicalize - canonicalize the apron representation of the abstract 
		//domain
		void canonicalize();

		/// assign_texpr_array - assign a expression to a set of variables
		void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest);
		
		/// join_array - the abstract value becomes the join of a set of
		/// abstract values
		void join_array(ap_environment_t * env, std::vector<Abstract*> X_pred);

		/// print - print the abstract domain on standard output
		void print(bool only_main = false);
};
#endif
