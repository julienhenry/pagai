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

	public:

		Abstract(ap_manager_t* _man);

		Abstract(Abstract* A);

		~Abstract();

		void set_top(ap_environment_t * env);

		void set_bottom(ap_environment_t * env);

		void change_environment(ap_environment_t * env);

		bool is_leq(Abstract * d);

		void widening(Node * n);

		void meet_tcons_array(ap_tcons1_array_t* tcons);

		void canonicalize();

		void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest);
		
		void join_array(ap_environment_t * env, std::vector<Abstract*> X_pred);

		void print();
};
#endif
