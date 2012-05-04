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

int k;

SMTlib::SMTlib() {

	k=0;
	stack_level = 0;

	int_type = new std::string("Int");
	float_type = new std::string("Real");

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
		char * argv[4];
		argv[0] = "";
		argv[1] = "-smt2";
		argv[2] = "-in";
		argv[3] = NULL;
		execvp("z3",argv);
	} else { 
		/* Parent : PAGAI */
		close(wpipefd[0]);
		close(rpipefd[1]);
	}

	//Enable model construction
	pwrite("(set-option :produce-models true)\n");
	pwrite("(set-option :interactive-mode true)\n");
	//pwrite("(set-logic QF_LRA)\n");

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
	DEBUG(*Out << "WRITTING : " << s  << "\n";);
	write(wpipefd[1], s.c_str(), strlen(s.c_str()));
}

int SMTlib::pread() {
	int ret;
	std::ostringstream oss;
	std::ostringstream filename;
	filename << "/tmp/return" << k << ".smt2";
	k++;

	std::ofstream tmp;
	tmp.open (filename.str().c_str());
	char buf;
	int parenthesis = 0;
	while (read(rpipefd[0], &buf, 1) > 0) {
		tmp << buf;
		oss << buf;
		if (buf == '(')
			parenthesis++;
		if (buf == ')')
			parenthesis--;
		if (buf == '\n' && parenthesis == 0) {
			break;
		}
	}
	
	tmp.close();
	DEBUG(
	*Out << "RECEIVED:\n" << oss.str() << "\n";
	*Out << "STORED " << filename.str() << "\n";
	);
//FILE * input = fdopen(rpipefd[0],"r");
	FILE * input = fopen (filename.str().c_str(), "r");
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
	std::string * res = new std::string("true");
	return res;
}

SMT_expr SMTlib::SMT_mk_false(){
	std::string * res = new std::string("false");
	return res;
}

SMT_var SMTlib::SMT_mk_bool_var(std::string name){
	if (!vars.count(name)) {
		std::string * res = new std::string(name);
		vars[name].var = res;
		vars[name].stack_level = stack_level;

		std::ostringstream oss;
		oss << "(declare-fun " << name << " () Bool)\n";
		pwrite(oss.str());
	}
	return vars[name].var;
}

SMT_var SMTlib::SMT_mk_var(std::string name, SMT_type type){
	if (!vars.count(name)) {
		std::string * res = new std::string(name);
		vars[name].var = res;
		vars[name].stack_level = stack_level;
		std::ostringstream oss;
		oss << "(declare-fun " << name << " () " << *((std::string*)type) << ")\n";
		pwrite(oss.str());
	}
	return vars[name].var;
}

SMT_expr SMTlib::SMT_mk_expr_from_bool_var(SMT_var var){
	std::string * res = new std::string(*((std::string*)var));
	return res;
}

SMT_expr SMTlib::SMT_mk_expr_from_var(SMT_var var){
	std::string * res = new std::string(*((std::string*)var));
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
	std::ostringstream oss;
	oss << "(ite "
		<< *((std::string*)c) << " "
		<< *((std::string*)t) << " "
		<< *((std::string*)e) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_not (SMT_expr a){
	std::ostringstream oss;
	oss << "(not " << *((std::string*)a) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_num (int n){
	std::ostringstream oss;
	if (n < 0)
		oss << "(- " << -n << ")";
	else
		oss << n;
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_num_mpq (mpq_t mpq) {
	std::ostringstream oss;
	if (mpq_sgn(mpq) < 0) {
		mpq_t nmpq;
		mpq_init(nmpq);
		mpq_neg(nmpq,mpq);
		oss << "(- " << nmpq << ")";
		mpq_clear(nmpq);
	} else {
		oss << mpq;
	}
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_real (double x) {
	std::ostringstream oss;
	if (x < 0)
		oss << "(- " << -x << ")";
	else
		oss << x;
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_sum (std::vector<SMT_expr> args){
	std::string * res;
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			res = new std::string(*((std::string*)args[0]));
			return res;
			break;
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::ostringstream oss;
			oss << "(+ "; 
			for (; b != e; ++b) {
				oss << *((std::string*)*b) << " ";
			}
			oss << ")";
			res = new std::string(oss.str());
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_sub (std::vector<SMT_expr> args){
	std::string * res;
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			res = new std::string(*((std::string*)args[0]));
			return res;
			break;
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::ostringstream oss;
			oss << "(- "; 
			for (; b != e; ++b) {
				oss << *((std::string*)*b) << " ";
			}
			oss << ")";
			res = new std::string(oss.str());
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_mul (std::vector<SMT_expr> args){
	std::string * res = new std::string("true");
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			res = new std::string(*((std::string*)args[0]));
			return res;
			break;
		default:
			std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
			std::ostringstream oss;
			oss << "(* "; 
			for (; b != e; ++b) {
				oss << *((std::string*)*b) << " ";
			}
			oss << ")";
			res = new std::string(oss.str());
			return res;
	}
}

SMT_expr SMTlib::SMT_mk_div (SMT_expr a1, SMT_expr a2) {
	std::ostringstream oss;
	oss << "(div " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	std::ostringstream oss;
	oss << "(mod " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	std::ostringstream oss;
	oss << "(xor " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	std::ostringstream oss;
	oss << "(< " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	std::ostringstream oss;
	oss << "(<= " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	std::ostringstream oss;
	oss << "(> " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
	return res;
}

SMT_expr SMTlib::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	std::ostringstream oss;
	oss << "(>= " << *((std::string*)a1) << " " << *((std::string*)a2) << ")";
	std::string * res = new std::string(oss.str());
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
	int ret;
	std::ostringstream oss;
	oss << "(assert "
		<< *((std::string*)a)
		<< ")\n";
	oss << "(check-sat)\n";
	DEBUG(
		*Out << "\n\n" << oss.str() << "\n\n";
	);
	pwrite(oss.str());

	ret = pread();
	if (ret == 1) {
		// SAT
		pwrite("(get-model)\n");
		pread();
		true_booleans->clear();
		true_booleans->insert(model.begin(),model.end());
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
