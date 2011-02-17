#include <map>

#include "llvm/BasicBlock.h"
#include "llvm/Type.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"


#include "ap_global1.h"

#include "Expr.h"
#include "apron.h"

std::map<Value *,ap_texpr1_t *> Exprs;

ap_texpr1_t * Expr::get_ap_expr(Node * n, Value * val) {
	if (Exprs.count(val) > 0) {
		if (Exprs[val] == NULL)
			fouts() << "NULL pointer in table Exprs !\n";
		return ap_texpr1_copy(Exprs[val]);
	} else {
		fouts() << "Missing apron expression for " << *val << "\n";
		/*val is not yet in the Expr map
		 * We have to create it */
		if (isa<Constant>(val)) {
			return create_ap_expr(n,dyn_cast<Constant>(val));
		} else {
			fouts() << "NOT IMPLEMENTED : get_ap_expr call "
					<< "with missing non constant expression\n";
			// NOT IMPLEMENTED
			//return ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),0);
			return NULL;
		}
	}
}

ap_texpr1_t * Expr::create_ap_expr(Node * n, Constant * val) {
	if (isa<ConstantInt>(val)) {
		ConstantInt * Int = dyn_cast<ConstantInt>(val);
		/*it is supposed we use signed int */
		int64_t n = Int->getSExtValue();
		return ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),n);
	} 
	if (isa<ConstantFP>(val)) {
		ConstantFP * FP = dyn_cast<ConstantFP>(val);
		// NOT IMPLEMENTED
		return NULL;
	}
	if (isa<UndefValue>(val)) {
		n->add_var(val);
		return ap_texpr1_copy(Exprs[val]);
	}
}

void Expr::set_ap_expr(Value * val, ap_texpr1_t * exp) {
	Exprs[val] = exp;
}

ap_texpr_rtype_t Expr::get_ap_type(Value * val) {
	ap_texpr_rtype_t res;
	
	switch (val->getType()->getTypeID()) {
	case Type::FloatTyID:
		res = AP_RTYPE_SINGLE;
		break;
	case Type::DoubleTyID:
		res = AP_RTYPE_DOUBLE;
		break;
	case Type::IntegerTyID:
		res = AP_RTYPE_INT;
		break;
	case Type::X86_FP80TyID:
		res = AP_RTYPE_EXTENDED;
		break;
	case Type::PPC_FP128TyID:
		res = AP_RTYPE_QUAD;
		break;
	default:
		fouts() << "Warning: Unknown type " << val->getType() << "\n";
		res = AP_RTYPE_REAL;
		break;
	}
	return res;
}
