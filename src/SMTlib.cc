#include <algorithm>
#include <cstddef>
#include <string.h>
#include <sstream>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include "llvm/Support/FormattedStream.h"

#include <gmp.h>
#include "SMTlib.h"
#include "Analyzer.h"
#include "Debug.h"

SMTlib::SMTlib() {
	pid_t cpid;
	char buf;

	if (pipe(rpipefd) == -1) {
		exit(EXIT_FAILURE);
	}
	if (pipe(wpipefd) == -1) {
		exit(EXIT_FAILURE);
	}

	cpid = fork();
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
		char * argv[3];
		argv[0] = "-smt2";
		argv[1] = "-in";
		argv[2] = NULL;
		execvp("z3",argv);
	} else { 
		/* Parent : PAGAI */
		close(wpipefd[0]);
		close(rpipefd[1]);
	}

	//Enable model construction
	pwrite("(set-option :produce-models true)\n");
	pwrite("(set-option :interactive-mode true)\n");
	pwrite("(set-logic QF_LIA)\n");

	//int_type = Z3_mk_int_sort(ctx);
	//float_type = Z3_mk_real_sort(ctx);
	//bool_type = Z3_mk_bool_sort(ctx);
}

SMTlib::~SMTlib() {
	close(wpipefd[1]); /* Reader will see EOF */
	close(rpipefd[0]);
	wait(NULL);
}

void SMTlib::pwrite(std::string s) {
	*Out << "WRITTING " << s ;
	write(wpipefd[1], s.c_str(), strlen(s.c_str()));
}

std::string SMTlib::pread() {
	char buf;
	std::ostringstream oss;
	while (read(rpipefd[0], &buf, 1) > 0) {
		oss << buf;
		*Out << "READ letter " << buf << "\n";
	}
	return oss.str();
}

SMT_expr SMTlib::SMT_mk_true(){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_false(){
	std::string * res = new std::string("false");
	return res;
}

SMT_var SMTlib::SMT_mk_bool_var(std::string name){
	std::string * res = new std::string(name);
	return res;
}

SMT_var SMTlib::SMT_mk_var(std::string name, SMT_type type){
	std::string * res = new std::string(name);
	return res;
}

SMT_expr SMTlib::SMT_mk_expr_from_bool_var(SMT_var var){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_expr_from_var(SMT_var var){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_or (std::vector<SMT_expr> args){
	std::string * res;
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			res = new std::string(*((std::string*)args[0]));
			return res;
			break;
		default:
			std::ostringstream oss;
			oss << "(or "; 

			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			for (; b != e; ++b) {
				oss << *((std::string*)*b) << " ";
			}
			oss << ")";
			res = new std::string(oss.str());
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_and (std::vector<SMT_expr> args){
	std::string * res;
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			res = new std::string(*((std::string*)args[0]));
			return res;
			break;
		default:
			std::ostringstream oss;
			oss << "(and "; 

			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			for (; b != e; ++b) {
				oss << *((std::string*)*b) << " ";
			}
			oss << ")";
			res = new std::string(oss.str());
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_eq (SMT_expr a1, SMT_expr a2){
	std::ostringstream oss;
	oss << "(= " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_diseq (SMT_expr a1, SMT_expr a2){
	std::ostringstream oss;
	oss << "(!= " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_not (SMT_expr a){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_num (int n){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_num_mpq (mpq_t mpq) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_real (double x) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_sum (std::vector<SMT_expr> args){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_sub (std::vector<SMT_expr> args){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_mul (std::vector<SMT_expr> args){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_div (SMT_expr a1, SMT_expr a2) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_int2real(SMT_expr a) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_real2int(SMT_expr a) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_is_int(SMT_expr a) {
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_int0() {
	std::string * res = new std::string("0");
	return res;
}

SMT_expr SMTlib::SMT_mk_real0() {
	std::string * res = new std::string("0");
	return res;
}

void SMTlib::SMT_print(SMT_expr a){
	*Out << *((std::string*)a) << "\n";
}

int SMTlib::SMT_check(SMT_expr a, std::set<std::string> * true_booleans){
	std::ostringstream oss;
	oss << "(assert "
		<< *((std::string*)a)
		<< ")\n";
	oss << "(check-sat)\n";
	*Out << "\n\n" << oss.str() << "\n\n";
	pwrite(oss.str());
	pread();
	return 0;
}

void SMTlib::push_context() {
	pwrite("(push 1)\n");
}

void SMTlib::pop_context() {
	pwrite("(pop 1)\n");
}
