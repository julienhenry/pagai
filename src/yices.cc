#include <cstddef>
#include <vector>
#include <sstream>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "yices_c.h"

#include "yices.h"
#include "Analyzer.h"

using namespace llvm;

yices::yices() {
	const char* intname =	"int";
	const char* floatname ="float";
	ctx = yices_mk_context();
	int_type = yices_mk_type(ctx,const_cast<char*>(intname));
	float_type = yices_mk_type(ctx,const_cast<char*>(floatname));
}

yices::~yices() {
	yices_del_context (ctx);
}

SMT_expr yices::SMT_mk_true() {
	return yices_mk_true (ctx);
}

SMT_expr yices::SMT_mk_false() {
	return yices_mk_false (ctx);
}

SMT_var yices::SMT_mk_bool_var(std::string val) {
	char * cstr = new char [val.size()+1];
	strcpy (cstr, val.c_str());
	SMT_var res;
	res = yices_get_var_decl_from_name(ctx,cstr);
	if (res == NULL) {
		res = yices_mk_bool_var_decl(ctx,cstr);
	} 
	return res;
}

SMT_var yices::SMT_mk_var(std::string name,SMT_type type) {
	char * cstr = new char [name.size()+1];
	strcpy (cstr, name.c_str());
	SMT_var res;
	res = yices_get_var_decl_from_name(ctx,cstr);
	if (res == NULL) {
		res = yices_mk_var_decl(ctx,cstr,type);
	} 
	return res;
}

SMT_expr yices::SMT_mk_expr_from_bool_var(SMT_var var) {
	return yices_mk_bool_var_from_decl (ctx,(yices_var_decl)var);
}

SMT_expr yices::SMT_mk_expr_from_var(SMT_var var) {
	return yices_mk_var_from_decl (ctx,(yices_var_decl)var);
}

SMT_expr yices::SMT_mk_or (std::vector<SMT_expr> args) {
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			return args[0];
			break;
		default:
			return yices_mk_or(ctx,&args[0],args.size());
	}
}

SMT_expr yices::SMT_mk_and (std::vector<SMT_expr> args) {
	switch (args.size()) {
		case 0:
			return SMT_mk_true();
			break;
		case 1:
			return args[0];
			break;
		default:
			return yices_mk_and(ctx,&args[0],args.size());
	}
}

SMT_expr yices::SMT_mk_eq (SMT_expr a1, SMT_expr a2) {
	return yices_mk_eq(ctx,(yices_expr)a1,(yices_expr)a2);
}

SMT_expr yices::SMT_mk_diseq (SMT_expr a1, SMT_expr a2) {
	return yices_mk_diseq(ctx,(yices_expr)a1,(yices_expr)a2);
}

SMT_expr yices::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e) {
	return yices_mk_ite(ctx,(yices_expr)c,(yices_expr)t,(yices_expr)e);
}

SMT_expr yices::SMT_mk_not (SMT_expr a) {
	return yices_mk_not(ctx,(yices_expr)a);
}

SMT_expr yices::SMT_mk_num (int n) {
	return yices_mk_num(ctx,n);
}

SMT_expr yices::SMT_mk_real (double x) {
	std::ostringstream oss;
	oss << x;
	SMT_var var = SMT_mk_var(oss.str(),float_type);
	return SMT_mk_expr_from_var(var);
}

SMT_expr yices::SMT_mk_sum (std::vector<SMT_expr> args) {
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return yices_mk_sum(ctx,&args[0],args.size());
	}
}

SMT_expr yices::SMT_mk_sub (std::vector<SMT_expr> args) {
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return yices_mk_sub(ctx,&args[0],args.size());
	}
}

SMT_expr yices::SMT_mk_mul (std::vector<SMT_expr> args) {
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return yices_mk_mul(ctx,&args[0],args.size());
	}
}

SMT_expr yices::SMT_mk_lt (SMT_expr a1, SMT_expr a2) {
	return yices_mk_lt(ctx,(yices_expr)a1,(yices_expr)a2);
} 

SMT_expr yices::SMT_mk_le (SMT_expr a1, SMT_expr a2) {
	return yices_mk_le(ctx,(yices_expr)a1,(yices_expr)a2);
}

SMT_expr yices::SMT_mk_gt (SMT_expr a1, SMT_expr a2) {
	return yices_mk_gt(ctx,(yices_expr)a1,(yices_expr)a2);
}

SMT_expr yices::SMT_mk_ge (SMT_expr a1, SMT_expr a2) {
	return yices_mk_ge(ctx,(yices_expr)a1,(yices_expr)a2);
}

void yices::SMT_print(SMT_expr a) {

	//Save position of current standard output
	fpos_t pos;
	fgetpos(stdout, &pos);
	int fd = dup(fileno(stdout));
	freopen("/tmp/yices_output.txt", "w", stdout);
	yices_pp_expr ((yices_expr)a);
	//Flush stdout so any buffered messages are delivered
	fflush(stdout);
	//Close file and restore standard output to stdout - which should be the terminal
	dup2(fd, fileno(stdout));
	close(fd);
	clearerr(stdout);
	fsetpos(stdout, &pos);

	FILE * tmp = fopen ("/tmp/yices_output.txt" , "r");
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;

	*Out << "\n";
}

bool yices::SMT_check(SMT_expr a, std::set<std::string> * true_booleans) {
	//yices_pp_expr ((yices_expr)a);
	yices_set_arith_only(1);
	yices_assert(ctx,(yices_expr)a);
	//*Out << "\n";
	if (yices_check(ctx) == l_true) {
		*Out << "sat\n";
		yices_var_decl_iterator it = yices_create_var_decl_iterator(ctx);
		yices_model m              = yices_get_model(ctx);
		

		//Save position of current standard output
		fpos_t pos;
		fgetpos(stdout, &pos);
		int fd = dup(fileno(stdout));
		freopen("/tmp/yices_output.txt", "w", stdout);
		yices_display_model(m);
		//Flush stdout so any buffered messages are delivered
		fflush(stdout);
		//Close file and restore standard output to stdout - which should be the terminal
		dup2(fd, fileno(stdout));
		close(fd);
		clearerr(stdout);
		fsetpos(stdout, &pos);

		FILE * tmp = fopen ("/tmp/yices_output.txt" , "r");
		fseek(tmp,0,SEEK_SET);
		char c;
		while ((c = (char)fgetc(tmp))!= EOF)
			*Out << c;

		*Out << "\n";
		
		while (yices_iterator_has_next(it)) {
			yices_var_decl d         = yices_iterator_next(it);
			//*Out <<  yices_get_var_decl_name(d) << " = ";
			std::string name (yices_get_var_decl_name(d));
			switch(yices_get_value(m, d)) {
				case l_true: 
					true_booleans->insert(name);
					//*Out << "true\n"; 
					break;
				case l_false: 
					//*Out << "false\n"; 
					break;
				case l_undef: 
					//*Out << "unknown\n"; 
					break;
			}
		}
		yices_del_iterator(it);
	} else {
		*Out << "unsat\n";
		return false;
	}
	return true;
}

void yices::push_context() {
	yices_push(ctx);
}

void yices::pop_context() {
	yices_pop(ctx);
}
