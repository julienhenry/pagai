#ifndef _ABSTRACTDISJ_H
#define _ABSTRACTDISJ_H

#include <vector>

#include "ap_global1.h"
#include "Abstract.h"
#include "AbstractMan.h"

class Node;
class Sigma;

/// Abstract Domain used for computing disjunctive invariants
class AbstractDisj: public Abstract {
	friend class Sigma;

	public:
		AbstractMan * man_disj;

	protected:
		std::vector<Abstract*> disj;
		
		void clear_all();

	public:
		/// create a disjunctive invariant with one sigle disjunct
		AbstractDisj(ap_manager_t* _man, ap_environment_t * env);

		/// create a disjunctive invariant with max_index+1 disjunct
		AbstractDisj(ap_manager_t* _man, ap_environment_t * env, int max_index);

		/// add a new disjunct in the abstract
		/// the method return the index of the new disjunct
		int AddDisjunct(ap_environment_t * env);
		
		/// get the disjunct of index 'index'
		Abstract * getDisjunct(int index);

		/// set the disjunct 'index' to the abstract A
		/// IMPORTANT: A should not duplicated, and should not be deleted
		void setDisjunct(int index, Abstract * A);

		/// verifies that the AbstractDisj has at least N disjunct
		/// if not, it creates disjuncts so that the AbstractDisj has N
		/// disjuncts
		void SetNDisjunct(int N);
		
		/// get the index of the last disjunct
		/// WARNING : the first index is 0. If the result of this method is N,
		/// it means the invariant has N+1 disjuncts, indexed from 0 to N
		int getMaxIndex();

		/// copy constructor : duplicates the abstract value
		AbstractDisj(Abstract* A);

		~AbstractDisj();

		/// set_top - the abstract domain is set to top
		void set_top(ap_environment_t * env);
		void set_top(ap_environment_t * env, int index);

		/// set_top - the abstract domain is set to bottom
		void set_bottom(ap_environment_t * env);
		void set_bottom(ap_environment_t * env, int index);

		/// change_environment - change the environment of the abstract value
		void change_environment(ap_environment_t * env);
		void change_environment(ap_environment_t * env, int index);

		bool is_leq_index(Abstract * d, int index);

		bool is_eq_index(Abstract * d, int index);

		/// is_bottom - return true iff the abstract value is at bottom
		bool is_bottom();
		bool is_bottom(int index);

		/// widening - applies the widening operator, according to its
		/// definition in the domain.
		void widening(Abstract * X);
		void widening(Abstract * X, int index);
		void widening_threshold(Abstract * X, ap_lincons1_array_t* cons);
		void widening_threshold(Abstract * X, ap_lincons1_array_t* cons, int index);

		/// meet_tcons_array - intersect the abstract domain with an array of
		/// constraints
		void meet_tcons_array(ap_tcons1_array_t* tcons);
		void meet_tcons_array(ap_tcons1_array_t* tcons, int index);

		/// canonicalize - canonicalize the apron representation of the abstract 
		//domain
		void canonicalize();

		/// assign_texpr_array - assign a expression to a set of variables
		void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest);

		void assign_texpr_array(
				ap_var_t* tvar, 
				ap_texpr1_t* texpr, 
				size_t size, 
				ap_abstract1_t* dest,
				int index);
		
		/// join_array - the abstract value becomes the join of a set of
		/// abstract values
		void join_array(ap_environment_t * env, std::vector<Abstract*> X_pred);
		void join_array(ap_environment_t * env, std::vector<Abstract*> X_pred, int index);

		void join_array_dpUcm(ap_environment_t *env, Abstract* n);
		void join_array_dpUcm(ap_environment_t *env, Abstract* n, int index);
		
		void meet(Abstract* A);

		/// to_tcons_array - convert the abstract value to a conjunction of
		// tree constraints
		ap_tcons1_array_t to_tcons_array();
		ap_tcons1_array_t to_tcons_array(int index);

		/// to_lincons_array - convert the abstract value to a conjunction of
		// linear constraints
		ap_lincons1_array_t to_lincons_array();
		ap_lincons1_array_t to_lincons_array(int index);

		/// print - print the abstract domain on standard output
		void print(bool only_main = false);

		void display(llvm::raw_ostream &stream, std::string * left = NULL) const;
};
#endif
