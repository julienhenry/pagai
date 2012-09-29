#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <set>

#include "ap_global1.h"
#include "Abstract.h"
#include "Live.h"
#include "Constraint.h"

using namespace llvm;

class Environment {

	private:
		ap_environment_t * env;


	public:
		Environment();
		Environment(const Environment &e);
		Environment(std::set<ap_var_t> * intvars, std::set<ap_var_t> * realvars);
		Environment(Abstract * A);
		Environment(ap_tcons1_array_t * cons);
		Environment(Constraint * cons);
		Environment(Constraint_array * cons);
		Environment(ap_environment_t * e);

		~Environment();

		// Overloaded copy assignment operator
		Environment & operator= (const Environment &e);

		ap_environment_t * getEnv();

		// insert into intdims the int dimensions of the environment
		// insert into realdims the real dimensions of the environment
		void get_vars(std::set<ap_var_t> * intdims, std::set<ap_var_t> * realdims);

		// same as get_vars, but gets only variables that are live in b
		void get_vars_live_in(
				BasicBlock * b, Live * LV,
				std::set<ap_var_t> * intdims, 
				std::set<ap_var_t> * realdims);

		// modifies exp1 and exp2, so that they have the same env
		static void common_environment(ap_texpr1_t * exp1, ap_texpr1_t * exp2);

		static Environment * common_environment(Expr* exp1, Expr* exp2);
		static Environment * common_environment(Environment* env1, Environment* env2);
		static Environment * intersection(Environment * env1, Environment * env2);

		void print(); 

	private:

		static ap_environment_t * common_environment(ap_environment_t * env1, ap_environment_t * env2);
};
#endif
