/**
 * \file SMT_manager.h
 * \brief Declaration of the SMT_manager class
 * \author Julien Henry
 */
#ifndef SMT_MANAGER_H
#define SMT_MANAGER_H

#include <string>
#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>

#include "gmp.h"
#include "mpfr.h"
#include "Debug.h"
#include "z3++.h"

/**
 * \class SMT_expr
 * \brief class of SMT expressions
 */
class SMT_expr {

	public:
		std::string s;
		boost::shared_ptr<z3::expr> z3;
		void * i;

		SMT_expr () {
			s = std::string("");
			z3.reset();
			i = NULL;
		}

		SMT_expr (const SMT_expr& e): s(e.s), i(e.i) {
			z3 = e.z3;
		}

		SMT_expr(z3::expr e) {
			s = std::string("");
			z3.reset(new z3::expr(e));
			i = NULL;
		}

		z3::expr * expr() {
			return z3.get();
		}

		~SMT_expr(){
			s.clear();
		}

		bool is_empty() {
			return i == NULL && z3.get() == NULL && s == "";
		}

		SMT_expr& operator=(const SMT_expr &e) {
			s.clear();
			s = e.s;
			z3 = e.z3;
			i = e.i;
			return *this;
		}
};

/**
 * \class SMT_type
 * \brief class of SMT types
 */
class SMT_type {
	public:
		std::string s;
		void* i;

		SMT_type () {
			s = std::string("");
			i = NULL;
		}

		SMT_type (const SMT_type& t): s(t.s), i(t.i) {
		}

		~SMT_type(){s.clear();}

		bool is_empty() {
			return i == NULL && s == "";
		}

		SMT_type& operator=(const SMT_type &t) {
			s.clear();
			s = t.s;
			i = t.i;
			return *this;
		}
};

/**
 * \class SMT_var
 * \brief class of SMT variables
 */
class SMT_var {
	public:
		std::string s;
		boost::shared_ptr<z3::symbol> z3;
		void* i;

		SMT_var () {
			s = std::string("");
			z3.reset();
			i = NULL;
		}

		SMT_var(z3::symbol n) {
			s = n.str();
			z3.reset(new z3::symbol(n));
			i = NULL;
		};

		~SMT_var() { }

		SMT_var& operator=(const SMT_var &v) {
			s.clear();
			s = v.s;
			z3 = v.z3;
			i = v.i;
			return *this;
		}

		z3::symbol * symb() {
			return z3.get();
		}

		int Compare (const SMT_var& v) const {
			if (i < v.i)
				return -1;
			else if (i > v.i)
				return 1;
			else {
				if (s < v.s)
					return -1;
				else if (s > v.s)
					return 1;
				else return 0;
			}
		}

		bool operator == (const SMT_var& v) const {
			return !Compare(v);
		}

		bool operator < (const SMT_var& v) const {
			return Compare(v)<0;   
		}
}; 

/**
 * \class SMT_manager
 * \brief interface of an SMT manager
 */
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
		virtual SMT_expr SMT_mk_xor (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e) = 0;
		virtual SMT_expr SMT_mk_not (SMT_expr a) = 0;

		virtual SMT_expr SMT_mk_num (int n) = 0;
		virtual SMT_expr SMT_mk_num_mpq (mpq_t mpq) = 0;
		virtual SMT_expr SMT_mk_real (double x) = 0;

		virtual SMT_expr SMT_mk_sum (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_sub (std::vector<SMT_expr> args) = 0;
		virtual SMT_expr SMT_mk_mul (std::vector<SMT_expr> args) = 0;

		virtual SMT_expr SMT_mk_sum (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_sub (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_mul (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_eq (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_diseq (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_lt (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_le (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_gt (SMT_expr a1, SMT_expr a2) = 0;
		virtual SMT_expr SMT_mk_ge (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_divides (SMT_expr a1, SMT_expr a2);

		virtual SMT_expr SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer = true) = 0;
		virtual SMT_expr SMT_mk_rem (SMT_expr a1, SMT_expr a2) = 0;

		virtual SMT_expr SMT_mk_int2real(SMT_expr a) = 0;
		virtual SMT_expr SMT_mk_real2int(SMT_expr a) = 0;
		virtual SMT_expr SMT_mk_is_int(SMT_expr a) = 0;

		virtual SMT_expr SMT_mk_int0() = 0;
		virtual SMT_expr SMT_mk_real0() = 0;

		virtual void push_context() = 0;
		virtual void pop_context() = 0;

		virtual void SMT_print(SMT_expr a) = 0;
		virtual void SMT_assert(SMT_expr a) = 0;
		virtual int SMT_check(SMT_expr a, std::set<std::string> * true_booleans) = 0;

		virtual bool interrupt();

		static std::vector<SMT_expr> vec2(SMT_expr a1, SMT_expr a2) {
			std::vector<SMT_expr> vec;
			vec.push_back(a1);
			vec.push_back(a2);
			return vec;
		}
};

#endif
