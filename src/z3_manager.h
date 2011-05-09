#ifndef Z3_MANAGER_H
#define Z3_MANAGER_H

#include <vector>
#include <map>
#include <set>
#include <string>

#include "z3.h"

#include "SMT_manager.h"

class z3_manager: public SMT_manager {

	private:
		Z3_context ctx;
		std::map<std::string,SMT_var> vars;
		std::map<SMT_var,SMT_type> types;
		Z3_sort bool_type;
	public:
		
		z3_manager();

		virtual ~z3_manager();

		SMT_expr SMT_mk_true();
		SMT_expr SMT_mk_false();

		SMT_var SMT_mk_bool_var(std::string name);
		SMT_var SMT_mk_var(std::string name,SMT_type type);
		SMT_expr SMT_mk_expr_from_bool_var(SMT_var var);
		SMT_expr SMT_mk_expr_from_var(SMT_var var);
		SMT_expr SMT_mk_or (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_and (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_eq (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_diseq (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e);
		SMT_expr SMT_mk_not (SMT_expr a);
		SMT_expr SMT_mk_num (int n);
		SMT_expr SMT_mk_real (double x);
		SMT_expr SMT_mk_sum (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_sub (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_mul (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_lt (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_le (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_gt (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_ge (SMT_expr a1, SMT_expr a2);

		void push_context();
		void pop_context();

		void SMT_print(SMT_expr a);
		bool SMT_check(SMT_expr a, std::set<std::string> * true_booleans);
};
#endif