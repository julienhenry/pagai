#include <map>

#include "llvm/BasicBlock.h"
#include "llvm/Type.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"


#include "ap_global1.h"

#include "Expr.h"
#include "apron.h"
#include "Debug.h"
#include "Analyzer.h"

std::map<Value *,ap_texpr1_t *> Exprs;

std::map<Value *,ap_texpr1_t *> Phivar_first_Expr;
std::map<Value *,ap_texpr1_t *> Phivar_last_Expr;

ap_texpr1_t * create_ap_expr(Node * n, Constant * val) {
	if (isa<ConstantInt>(val)) {
		ConstantInt * Int = dyn_cast<ConstantInt>(val);
		// it is supposed we use signed int
		int64_t n = Int->getSExtValue();
		Exprs[val] = ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),n);
	} 
	if (isa<ConstantFP>(val)) {
		ConstantFP * FP = dyn_cast<ConstantFP>(val);
		double x = FP->getValueAPF().convertToDouble();
		Exprs[val] = ap_texpr1_cst_scalar_double(ap_environment_alloc_empty(),x);
	}
	if (isa<ConstantPointerNull>(val)) {
		Exprs[val] = ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),0);
	}
	if (isa<UndefValue>(val)) {
		n->add_var(val);
	}
	return Exprs[val];
}

ap_texpr1_t * get_ap_expr(Node * n, Value * val) {
	if (Exprs.count(val) > 0) {
		if (Exprs[val] == NULL)
			*Out << "ERROR: NULL pointer in table Exprs !\n";
		return Exprs[val];
	} else {
		// val is not yet in the Expr map
		// We have to create it
		if (isa<Constant>(val)) {
			return create_ap_expr(n,dyn_cast<Constant>(val));
		} else {
			return NULL;
		}
	}
}


ap_texpr1_t * get_phivar_previous_expr(Value * val) {
	if (Phivar_first_Expr.count(val)) {
		return Phivar_first_Expr[val];
	}
	return NULL;
}

void set_phivar_previous_expr(Value * val, ap_texpr1_t * exp) {
	if (Phivar_first_Expr.count(val)) {
		Phivar_first_Expr.erase(val);
	}
	Phivar_first_Expr[val] = exp;
}

void set_ap_expr(Value * val, ap_texpr1_t * exp) {
	if (Exprs.count(val) && !Phivar_first_Expr.count(val)) {
		Exprs.erase(val);
	}
	Exprs[val] = exp;
}

ap_environment_t * common_environment(
	ap_environment_t * env1, 
	ap_environment_t * env2) {

	ap_dimchange_t * dimchange1 = NULL;
	ap_dimchange_t * dimchange2 = NULL;
	ap_environment_t * lcenv = ap_environment_lce(
		env1,
		env2,
		&dimchange1,
		&dimchange2);

	if (dimchange1 != NULL)
		ap_dimchange_free(dimchange1);
	if (dimchange2 != NULL)
		ap_dimchange_free(dimchange2);

	return lcenv;
}


/// common_environment = modifies exp1 and exp2 such that 
/// they have the same common environment
///
void common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2) {

	// we compute the least common environment for the two expressions
	ap_dimchange_t * dimchange1 = NULL;
	ap_dimchange_t * dimchange2 = NULL;
	ap_environment_t* lcenv = ap_environment_lce(
			(*exp1)->env,
			(*exp2)->env,
			&dimchange1,
			&dimchange2);
	// we extend the environments such that both expressions have the same one
	*exp1 = ap_texpr1_extend_environment(*exp1,lcenv);
	*exp2 = ap_texpr1_extend_environment(*exp2,lcenv);
	if (dimchange1 != NULL)
		ap_dimchange_free(dimchange1);
	if (dimchange2 != NULL)
		ap_dimchange_free(dimchange2);
	ap_environment_free(lcenv);
}

int get_ap_type(Value * val,ap_texpr_rtype_t &ap_type) {
	
	switch (val->getType()->getTypeID()) {
	case Type::FloatTyID:
		ap_type = AP_RTYPE_REAL;
		break;
	case Type::DoubleTyID:
		ap_type = AP_RTYPE_REAL;
		break;
	case Type::IntegerTyID:
		ap_type = AP_RTYPE_INT;
		break;
	case Type::X86_FP80TyID:
		ap_type = AP_RTYPE_REAL;
		break;
	case Type::PPC_FP128TyID:
		ap_type = AP_RTYPE_REAL;
		break;
	default:
		DEBUG(
		*Out << "Warning: Unknown type for " << *val << "\n";
		Out->flush();
		)
		ap_type = AP_RTYPE_REAL;
		return 1;
	}
	return 0;
}

void environment_print(ap_environment_t * env) {

	FILE* tmp = tmpfile();

	ap_environment_fdump(tmp,env);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
}

void texpr1_print(ap_texpr1_t * expr) {

	FILE* tmp = tmpfile();

	ap_texpr1_fprint(tmp,expr);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
}

void tcons1_array_print(ap_tcons1_array_t * cons) {

	FILE* tmp = tmpfile();

	ap_tcons1_array_fprint(tmp,cons);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
}
