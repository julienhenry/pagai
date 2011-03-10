#include <cstddef>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"

#include "yices_c.h"

#include "yices.h"


SMT_expr yices::SMT_mk_true() {
	return yices_mk_true (ctx);
}

SMT_expr yices::SMT_mk_false() {
	return yices_mk_false (ctx);
}

SMT_var yices::SMT_mk_bool_var(std::string val) {
	char * cstr = new char [val.size()+1];
	strcpy (cstr, val.c_str());
	return yices_mk_bool_var_decl(ctx,cstr);
}

SMT_expr yices::SMT_mk_expr_from_bool_var(SMT_var var) {
	return yices_mk_bool_var_from_decl (ctx,(yices_var_decl)var);
}

SMT_expr yices::SMT_mk_or (std::vector<SMT_expr> args) {
	switch (args.size()) {
		case 0:
			return NULL;
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
			return NULL;
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
	yices_pp_expr ((yices_expr)a);
}
