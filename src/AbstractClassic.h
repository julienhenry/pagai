#ifndef _ABSTRACTCLASSIC_H
#define _ABSTRACTCLASSIC_H

#include <vector>

#include "ap_global1.h"
#include "Abstract.h"

class Node;
class AbstractGopan;

/// Abstract Domain, used by every AI pass but AIGopan
class AbstractClassic: public Abstract {

	protected:
		void clear_all();

	public:

		AbstractClassic(ap_manager_t* _man, Environment * env);

		/// copy constructor : duplicates the abstract domain
		AbstractClassic(Abstract* A);

		~AbstractClassic();

		/// set_top - the abstract domain is set to top
		void set_top(Environment * env);

		/// set_top - the abstract domain is set to bottom
		void set_bottom(Environment * env);

		/// change_environment - change the environment of the abstract value
		void change_environment(Environment * env);

		/// is_bottom - return true iff the abstract value is at bottom
		bool is_bottom();

		/// is_top - return true iff the abstract value is at top
		bool is_top();

		/// widening - applies the widening operator, according to its
		/// definition in the domain.
		void widening(Abstract * X);
		void widening_threshold(Abstract * X, Constraint_array* cons);

		/// meet_tcons_array - intersect the abstract domain with an array of
		/// constraints
		void meet_tcons_array(Constraint_array* tcons);

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
		void join_array(Environment * env, std::vector<Abstract*> X_pred);

		void join_array_dpUcm(Environment *env, Abstract* n);

		void meet(Abstract* A);
		
		/// to_tcons_array - convert the abstract value to a conjunction of
		// tree constraints
		ap_tcons1_array_t to_tcons_array();

		/// to_lincons_array - convert the abstract value to a conjunction of
		// linear constraints
		ap_lincons1_array_t to_lincons_array();

		/// print - print the abstract domain on standard output
		void print(bool only_main = false);

		void display(llvm::raw_ostream &stream, std::string * left = NULL) const;
};
#endif
