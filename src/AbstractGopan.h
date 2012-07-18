#ifndef _ABSTRACTGOPAN_H
#define _ABSTRACTGOPAN_H

#include <vector>

#include "ap_global1.h"
#include "Abstract.h"

class Node;
class Abstract;

/// Abstract Domain used by Lookahead Widening algorithm
class AbstractGopan : public Abstract {

	private:
		void clear_all();

	public:

		AbstractGopan(ap_manager_t* _man, ap_environment_t * env);

		/// copy constructor : duplicates the abstract domain
		AbstractGopan(Abstract* A);

		~AbstractGopan();

		/// set_top - the abstract domain is set to top
		void set_top(ap_environment_t * env);

		/// set_top - the abstract domain is set to bottom
		void set_bottom(ap_environment_t * env);

		/// change_environment - change the environment of the abstract value
		void change_environment(ap_environment_t * env);

		/// is_bottom - return true iff this is bottom
		bool is_bottom();

		/// widening - applies the widening operator, according to its
		/// definition in the domain.
		void widening(Abstract * X);

		void widening_threshold(Abstract * X, ap_lincons1_array_t* cons);

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
		void join_array_dpUcm(ap_environment_t *env, Abstract* n);

		void meet(Abstract* A);

		/// to_tcons_array - convert the abstract value to a conjunction of
		// tree constraints
		ap_tcons1_array_t to_tcons_array();

		/// to_lincons_array - convert the abstract value to a conjunction of
		// linear constraints
		ap_lincons1_array_t to_lincons_array();

		/// print - print the abstract domain on standard output
		void print(bool only_main = false);

		void display(llvm::raw_ostream &stream) const;
};
#endif
