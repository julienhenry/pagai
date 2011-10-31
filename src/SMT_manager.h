#ifndef SMT_MANAGER_H
#define SMT_MANAGER_H

#include <string>
#include <vector>
#include <set>
#include "gmp.h"
#include "mpfr.h"

typedef void* SMT_expr;
typedef void* SMT_type;
typedef void* SMT_var;
typedef void* SMT_model;


class SMT_manager {
	public:
		SMT_type int_type;
		SMT_type float_type;
	public:
		virtual ~SMT_manager() {}

		virtual SMT_expr SMT_mk_true() = 0;
		virtual SMT_expr SMT_mk_false() = 0;

		virtual SMT_var SMT_mk_bool_var(std::string name) = 0;
		virtual SMT_var SMT_mk_var(std::string name,SMT_type type) = 0;
		virtual SMT_expr SMT_mk_expr_from_bool_var(SMT_var var) = 0;
		virtual SMT_expr SMT_mk_expr_from_var(SMT_var var) = 0;
		virtual SMT_expr SMT_mk_or (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_and (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_eq (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_diseq (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e) = 0;
		virtual SMT_expr SMT_mk_not (SMT_expr a) = 0;
		virtual SMT_expr SMT_mk_num (int n) = 0;
		virtual SMT_expr SMT_mk_num_mpq (mpq_t mpq) = 0;
		virtual SMT_expr SMT_mk_real (double x) = 0;
		virtual SMT_expr SMT_mk_sum (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_sub (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_mul (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_lt (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_le (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_gt (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_ge (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_div (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_rem (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_xor (SMT_expr a1, SMT_expr a2) = 0;

		virtual void push_context() = 0;
		virtual void pop_context() = 0;

		virtual void SMT_print(SMT_expr a) = 0;
		virtual int SMT_check(SMT_expr a, std::set<std::string> * true_booleans) = 0;

};

#endif
