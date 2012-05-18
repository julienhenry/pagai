#ifndef SMTLIB_H
#define SMTLIB_H

#include <vector>
#include <map>
#include <set>
#include <string>

#include "SMT_manager.h"

#define LOG_SMT 1
#define MATHSAT 0
#define Z3 1
#define SMTINTERPOL 0

#if MATHSAT || SMTINTERPOL
#define SMT_SUPPORTS_DIVIDES 1
#endif

class SMTlib: public SMT_manager {

	private:

		struct definedvars {
			SMT_var var;
			int stack_level;
		};

		std::map<std::string,struct definedvars> vars;

		std::set<std::string> model;

		int stack_level;

		int wpipefd[2]; // pipe from PAGAI to the SMT solver
		int rpipefd[2]; // pipe from the SMT solver to PAGAI
		FILE *input;

		void pwrite(std::string s);
		int pread();
	
		FILE *log_file;

	public:
		
		SMTlib();

		virtual ~SMTlib();

		SMT_expr SMT_mk_true();
		SMT_expr SMT_mk_false();

		SMT_var SMT_mk_bool_var(std::string name);
		SMT_var SMT_mk_var(std::string name,SMT_type type);
		SMT_expr SMT_mk_expr_from_bool_var(SMT_var var);
		SMT_expr SMT_mk_expr_from_var(SMT_var var);

		SMT_expr SMT_mk_or (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_and (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_xor (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e);
		SMT_expr SMT_mk_not (SMT_expr a);

		SMT_expr SMT_mk_num (int n);
		SMT_expr SMT_mk_num_mpq (mpq_t mpq);
		SMT_expr SMT_mk_real (double x);

		SMT_expr SMT_mk_sum (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_sub (std::vector<SMT_expr> args);
		SMT_expr SMT_mk_mul (std::vector<SMT_expr> args);

		SMT_expr SMT_mk_eq (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_diseq (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_lt (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_le (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_gt (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_ge (SMT_expr a1, SMT_expr a2);

#if SMT_SUPPORTS_DIVIDES
		SMT_expr SMT_mk_divides (SMT_expr a1, SMT_expr a2);
#endif

		SMT_expr SMT_mk_div (SMT_expr a1, SMT_expr a2);
		SMT_expr SMT_mk_rem (SMT_expr a1, SMT_expr a2);

		SMT_expr SMT_mk_int2real(SMT_expr a);
		SMT_expr SMT_mk_real2int(SMT_expr a);
		SMT_expr SMT_mk_is_int(SMT_expr a);

		SMT_expr SMT_mk_int0();
	    SMT_expr SMT_mk_real0();

		void push_context();
		void pop_context();

		void SMT_print(SMT_expr a);
		int SMT_check(SMT_expr a, std::set<std::string> * true_booleans);
};
#endif
