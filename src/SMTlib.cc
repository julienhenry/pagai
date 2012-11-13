/**
 * \file SMTlib.cc
 * \brief Implementation of the SMTlib class
 * \author Julien Henry
 */
#include <algorithm>
#include <cstddef>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <unistd.h>

#include "llvm/Support/FormattedStream.h"

#include <gmp.h>
#include "SMTlib.h"
#include "Analyzer.h"
#include "Debug.h"
#include "SMTlib2driver.h"


SMTlib::SMTlib() {
	if (log_smt_into_file()) {
		static int logfile_counter = 0;
		char filename[sizeof("logfile-000.smt2")];
		sprintf(filename, "logfile-%03d.smt2", logfile_counter++);
		log_file = fopen(filename, "w");
	} else {
		log_file = NULL;
	}

	stack_level = 0;

	int_type.s = "Int";
	float_type.s = "Real";
	char buf;
	solver_pid = 0;

	if (pipe(rpipefd) == -1) {
		exit(EXIT_FAILURE);
	}
	if (pipe(wpipefd) == -1) {
		exit(EXIT_FAILURE);
	}

	pid_t cpid = fork();
	if (cpid == -1) {
		exit(EXIT_FAILURE);
	}
	if (cpid == 0) {
		/* Child : SMT solver */
		close(wpipefd[1]);
		close(rpipefd[0]);
		dup2(wpipefd[0], STDIN_FILENO);
		dup2(rpipefd[1], STDOUT_FILENO);
		close(wpipefd[0]);
		close(rpipefd[1]);
		switch (getSMTSolver()) {
			case MATHSAT:
				char * mathsat_argv[2];
				mathsat_argv[0] = (char*) "mathsat";
				mathsat_argv[1] = NULL;
				if (execvp("mathsat",mathsat_argv)) {
					perror("exec mathsat");
					exit(1);
				}
				break;
			case SMTINTERPOL:
				char * smtinterpol_argv[2];
				smtinterpol_argv[0] = (char*) "smtinterpol";
				smtinterpol_argv[1] = NULL;
				if (execvp("smtinterpol",smtinterpol_argv)) {
					perror("exec smtinterpol");
					exit(1);
				}		
				break;
			case Z3:
			case Z3_QFNRA:
				char * z3_argv[4];
				z3_argv[0] = (char*)"z3";
				z3_argv[1] = (char*)"-smt2";
				z3_argv[2] = (char*)"-in";
				z3_argv[3] = NULL;
				if (execvp("z3",z3_argv)) {
					perror("exec z3");
					exit(1);
				}
				break;
			case CVC3:
				char * cvc3_argv[4];
				cvc3_argv[0] = (char*)"cvc3";
				cvc3_argv[1] = (char*)"-lang";
				cvc3_argv[2] = (char*)"smt2";
				cvc3_argv[3] = NULL;
				if (execvp("cvc3",cvc3_argv)) {
					perror("exec cvc3");
					exit(1);
				}
				break;
			default:
				exit(1);
		}
	}

	/* Parent : PAGAI */
	solver_pid = cpid;
	close(wpipefd[0]);
	close(rpipefd[1]);
	input = fdopen(rpipefd[0],"r");
	//setbuf(input, NULL);
	if (input == NULL) {
		perror("fdopen");
		exit(1);
	}

	//Enable model construction
	if (getSMTSolver() == CVC3) {
		pwrite("(set-logic AUFLIRA)\n");
	} else {
		pwrite("(set-option :produce-models true)\n");
		pwrite("(set-option :produce-unsat-cores true)\n");
		if (getSMTSolver() == Z3 || getSMTSolver() == Z3_QFNRA) {
			pwrite("(set-option :interactive-mode true)\n");
			if (getTimeout() != 0) {
				std::ostringstream timeout;
				timeout << getTimeout()*1000;
				pwrite("(set-option :soft-timeout "+timeout.str()+")\n");
			}
		}
		if (getSMTSolver() == SMTINTERPOL) {
			pwrite("(set-logic QF_UFLIRA)\n");
		}
		pwrite("(set-option :print-success false)\n");
	}
	//pwrite("(set-logic QF_LRA)\n");
}

SMTlib::~SMTlib() {
	pwrite("(exit)\n");
	close(wpipefd[1]); /* Reader will see EOF */
	close(rpipefd[0]);
	if (log_file) fclose(log_file);
	wait(NULL);
}

void SMTlib::pwrite(std::string s) {
	DEBUG(*Out << "WRITING : " << s  << "\n";);
	// DM pquoi pas s.length() ?
	if (!write(wpipefd[1], s.c_str(), strlen(s.c_str()))) {
		*Out << "ERROR WHEN TRYING TO WRITE IN THE SMT-LIB PIPE\n";
	}
	if (log_file) {
		fputs(s.c_str(), log_file);
		fflush(log_file);
	}
}

int SMTlib::pread() {
	int ret;

	SMTlib2driver driver;
	driver.parse(input);

	switch (driver.ans) {
		case SAT:
			ret = 1;
			break;
		case UNSAT:
			ret = 0;
			break;
		case UNKNOWN:
			*Out << "UNKNOWN\n";
			ret = -1;
			break;
		default:
			ret = -1;
	}

	model.clear();
	model.insert(driver.model.begin(),driver.model.end());
	return ret;
}

SMT_expr SMTlib::SMT_mk_true(){
	SMT_expr res;
	res.s = std::string("true");
	res.i = NULL;
	return res;
}

SMT_expr SMTlib::SMT_mk_false(){
	SMT_expr res;
	res.s = std::string("false");
	res.i = NULL;
	return res;
}

SMT_var SMTlib::SMT_mk_bool_var(std::string name){
	if (!vars.count(name)) {
		SMT_var res;
		res.s = name;
		vars[name].var = res;
		vars[name].stack_level = stack_level;

		std::ostringstream oss;
		pwrite("(declare-fun " + name + " () Bool)\n");
	}
	return vars[name].var;
}

SMT_var SMTlib::SMT_mk_var(std::string name, SMT_type type){
	if (!vars.count(name)) {
		SMT_var res;
		res.s = name;
		vars[name].var = res;
		vars[name].stack_level = stack_level;
		pwrite("(declare-fun " + name + " () " + type.s + ")\n");
	}
	return vars[name].var;
}

SMT_expr SMTlib::SMT_mk_expr_from_bool_var(SMT_var var){
	SMT_expr res;
	res.s = var.s;
	res.i = NULL;
	return res;
}

SMT_expr SMTlib::SMT_mk_expr_from_var(SMT_var var){
	SMT_expr res;
	res.s = var.s;
	res.i = NULL;
	return res;
}

SMT_expr SMTlib::SMT_mk_or (std::vector<SMT_expr> args){
	SMT_expr res;
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			res.s = args[0].s;
			return res;
			break;
		default:
			std::string or_smt;
			or_smt = "(or "; 

			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			for (; b != e; ++b) {
				or_smt += (*b).s + " ";
			}
			or_smt += ")";
			res.s = or_smt;
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_and (std::vector<SMT_expr> args){
	SMT_expr res;
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			res.s = args[0].s;
			return res;
			break;
		default:
			std::string or_smt;
			or_smt = "(and \n"; 

			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			for (; b != e; ++b) {
				or_smt += "     " + (*b).s + "\n";
			}
			or_smt += "     )";
			res.s = or_smt;
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_eq (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.s = "(= " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_diseq (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.s = "(not (= " + a1.s + " " + a2.s + "))";
	return res;
}

SMT_expr SMTlib::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e){
	SMT_expr res;
	res.s = "(ite "
		+ c.s + " "
		+ t.s + " "
		+ e.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_not (SMT_expr a){
	SMT_expr res;
	res.s = "(not " + a.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_num (int n){
	SMT_expr res;
	std::ostringstream oss;
        //std::cerr << "const " << n << std::endl;
        if (n == -2147483648)
                oss << "(- 2147483648)";
	else if (n < 0)
		oss << "(- " << -n << ")";
	else
		oss << n;
	res.s = oss.str();
	return res;
}

SMT_expr SMTlib::SMT_mk_num_mpq (mpq_t mpq) {
	SMT_expr res;
	//std::ostringstream oss;
	char * charmpq;
	if (mpq_sgn(mpq) < 0) {
		mpq_t nmpq;
		mpq_init(nmpq);
		mpq_neg(nmpq,mpq);
		charmpq = __gmpq_get_str(NULL,10,nmpq);
		std::string s(charmpq);
		res.s = "(- " + s + ")";
		//oss << "(- " << nmpq << ")";
		mpq_clear(nmpq);
	} else {
		charmpq = __gmpq_get_str(NULL,10,mpq);
		res.s = charmpq;
		//oss << mpq;
	}
	//res.s = oss.str();
	return res;
}

std::string num_to_string(std::string num,int exponent) {
	std::ostringstream oss;
	switch (exponent) {
		case 0:
			oss << "0." << num;
			break;
		default:
			std::string r;
			if (exponent > 0) {
				int k;
				for (k = 0; k < exponent; k++) {
					if (k >= num.size())
						oss << "0";
					else
						oss << num[k];
				}
				oss << ".";
				if (k >= num.size())
					oss << "0";
				else
					for (; k < num.size(); k++) {
						oss << num[k];
					}
			} else {
				r = "0.";
				for (int i = -exponent; i > 1; i--) {
					r.append("0");
				}
				r.append("1");
				oss << "(* 0." << num << " " << r << ")";
			}
	}
	return oss.str();
}

SMT_expr SMTlib::SMT_mk_real (double x) {
	SMT_expr res;
	std::ostringstream oss;
	double intpart;
	bool is_neg = false;
	bool is_zero = false;

	mpf_t f;
	mpf_init(f);
	mpf_set_d(f,x);
	switch (mpf_sgn(f)) {
		case 0:
			is_zero = true;
			break;
		case -1:
			is_neg = true;
			mpf_t fneg;
			mpf_init(fneg);
			mpf_abs(fneg,f);
			mpf_set(f,fneg);
			mpf_clear(fneg);
			break;
		default:
			break;
	}
	mp_exp_t expptr;
	char * r = mpf_get_str(NULL,&expptr,10,0,f);
	mpf_clear(f);
	std::string num(r);
	
	if (is_zero)
		oss << "0.0";
	else {
		if (is_neg) oss << "(- "; 
		oss << num_to_string(num,expptr);
		if (is_neg) oss << ")"; 
		
	}
	res.s = oss.str();
	return res;
}

SMT_expr SMTlib::SMT_mk_sum (std::vector<SMT_expr> args){
	SMT_expr res;
	switch (args.size()) {
		case 0:
			res.s = std::string("");
			break;
		case 1:
			res.s = args[0].s;
			break;
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::string r;
			r = "(+ "; 
			for (; b != e; ++b) {
				r += (*b).s + " ";
			}
			r += ")";
			res.s = r;
	}
	return res;
}

SMT_expr SMTlib::SMT_mk_sub (std::vector<SMT_expr> args){
	SMT_expr res;
	switch (args.size()) {
		case 0:
			res.s = std::string("");
			break;
		case 1:
			res.s = args[0].s;
			return res;
			break;
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::string r;
			r = "(- "; 
			for (; b != e; ++b) {
				r += (*b).s + " ";
			}
			r += ")";
			res.s = r;
	}
	return res;
}

SMT_expr SMTlib::SMT_mk_mul (std::vector<SMT_expr> args){
	SMT_expr res;
	switch (args.size()) {
		case 0:
			res.s = std::string("");
			break;
		case 1:
			res.s = args[0].s;
			return res;
			break;
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::string r;
			r = "(* "; 
			for (; b != e; ++b) {
				r += (*b).s + " ";
			}
			r += ")";
			res.s = r;
	}
	return res;
}

SMT_expr SMTlib::SMT_mk_sum (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.s = "(+ " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_sub (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.s = "(- " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_mul (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.s = "(* " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer) {
	SMT_expr res;
	// the syntax in SMTlib 2 differs between integer and real division
	if (integer)
		res.s = "(div ";
	else
		res.s = "(/ ";
	res.s += a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.s = "(mod " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.s = "(xor " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.s = "(< " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.s = "(<= " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.s = "(> " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.s = "(>= " + a1.s + " " + a2.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_int2real(SMT_expr a) {
	SMT_expr res;
	res.s = "(to_real " + a.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_real2int(SMT_expr a) {
	SMT_expr res;
	res.s = "(to_int " + a.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_is_int(SMT_expr a) {
	SMT_expr res;
	res.s = "(is_int " + a.s + ")";
	return res;
}

SMT_expr SMTlib::SMT_mk_int0() {
	SMT_expr res;
	res.s = std::string("0");
	return res;
}

SMT_expr SMTlib::SMT_mk_real0() {
	SMT_expr res;
	res.s = std::string("0.0");
	return res;
}

#if SMT_SUPPORTS_DIVIDES
// WORKS ONLY FOR CONSTANT a2
SMT_expr SMTlib::SMT_mk_divides (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.s = "((_ divisible " +  a1.s + ") " +  a2.s + ")";
	return res;
}
#endif

void SMTlib::SMT_print(SMT_expr a){
	*Out << a.s << "\n";
}

void SMTlib::SMT_assert(SMT_expr a){
	std::string assert_stmt;
	assert_stmt = "(assert " + a.s + ")\n";
	DEBUG(
			*Out << "\n\n" << assert_stmt << "\n\n";
		 );
	pwrite(assert_stmt);
}

int SMTlib::SMT_check(SMT_expr a, std::set<std::string> * true_booleans){
	int ret;
	std::string check_stmt;
	check_stmt = "(assert " + a.s + ")\n";
	if (getSMTSolver() == Z3_QFNRA) {
		check_stmt += "(check-sat-using qfnra)\n";
	} else {
		check_stmt += "(check-sat)\n";
	}
	DEBUG(
			*Out << "\n\n" << check_stmt << "\n\n";
		 );
	pwrite(check_stmt);

	ret = pread();
	if (ret == 1) {
		// SAT
		pwrite("(get-model)\n");
		pread();
		true_booleans->clear();
		true_booleans->insert(model.begin(),model.end());
	}
	if (ret == 0) {
		// UNSAT
		pwrite("(get-unsat-core)\n");
		pread();
	}
	return ret;
}

void SMTlib::push_context() {
	pwrite("(push 1)\n");
	stack_level++;
}

void SMTlib::pop_context() {
	pwrite("(pop 1)\n");
	stack_level--;

	std::map<std::string,struct definedvars> tmpvars;
	std::map<std::string,struct definedvars>::iterator it = vars.begin(), et = vars.end();
	for (; it != et; it++) {
		if ((*it).second.stack_level <= stack_level) {
			tmpvars.insert(*it);
		}
	}
	vars.clear();
	vars.insert(tmpvars.begin(), tmpvars.end());
}

bool SMTlib::interrupt() {
  kill(solver_pid, SIGINT);
  return true;
}
