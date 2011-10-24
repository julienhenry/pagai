#include <map>

#include "Expr.h"
#include "apron.h"
#include "Debug.h"
#include "Analyzer.h"
#include "AIpass.h"
#include "Live.h"


ap_texpr1_t * create_ap_expr(Node * n, Constant * val) {
	if (isa<ConstantInt>(val)) {
		ConstantInt * Int = dyn_cast<ConstantInt>(val);
		// it is supposed we use signed int
		int64_t i = Int->getSExtValue();
		n->Exprs[val] = ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),i);
	} 
	if (isa<ConstantFP>(val)) {
		ConstantFP * FP = dyn_cast<ConstantFP>(val);
		double x = FP->getValueAPF().convertToDouble();
		if (!FP->isExactlyValue(x)) {
			float f = FP->getValueAPF().convertToFloat(); 
			x = f;
		}
		n->Exprs[val] = ap_texpr1_cst_scalar_double(ap_environment_alloc_empty(),x);
	}
	if (isa<ConstantPointerNull>(val)) {
		n->Exprs[val] = ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),0);
	}
	if (isa<UndefValue>(val)) {
		n->add_var(val);
	}
	if (n->Exprs.count(val) == 0) {
		n->add_var(val);
	}

	return n->Exprs[val];
}

void clear_all_exprs(Node * n) {
	std::map<Value *,ap_texpr1_t *>::iterator it = n->Exprs.begin(), et = n->Exprs.end();
	for (;it != et; it++) {
		ap_texpr1_free((*it).second);
	}
}

ap_texpr1_t * get_ap_expr_rec(Node * n, Value * val, std::set<Node*> * seen) {
	ap_texpr1_t * res = NULL;
	if (n->Exprs.count(val) > 0) {
		res = n->Exprs[val];
	} else if (!CurrentAIpass->LV->isLiveByLinearityInBlock(val,n->bb)) {
		res = NULL;
	} else if (seen->count(n) == 0) {
		seen->insert(n);
		std::set<BasicBlock*> preds = CurrentAIpass->getPredecessors(n->bb);
		std::set<BasicBlock*>::iterator it = preds.begin(), et = preds.end();
		for (; it != et; it++) {
			res = get_ap_expr_rec(Nodes[*it],val,seen);
			if (res != NULL) break;
		}
	}
	seen->insert(n);
	return res;
}

ap_texpr1_t * get_ap_expr(Node * n, Value * val) {
	if (isa<UndefValue>(val)) {
		return create_ap_expr(n,dyn_cast<Constant>(val));
	}
	if (isa<Constant>(val)) {
		return create_ap_expr(n,dyn_cast<Constant>(val));
	}
	std::set<Node*> seen;
	ap_texpr1_t * res = get_ap_expr_rec(n,val,&seen);
	if (res == NULL) {
		// val is not yet in the Expr map
		// We have to create it
		n->add_var(val);
		return n->Exprs[val];
	}
	return res;
}

void set_ap_expr(Value * val, ap_texpr1_t * exp, Node * n) {
	if (n->Exprs.count(val)) {
		n->Exprs.erase(val);
	}
	n->Exprs[val] = exp;
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


ap_environment_t * intersect_environment(
	ap_environment_t * env1, 
	ap_environment_t * env2) {
	ap_environment_t * lcenv = common_environment(env1,env2);
	ap_environment_t * intersect = ap_environment_copy(lcenv);	

	for (size_t i = 0; i < lcenv->intdim + lcenv->realdim; i++) {
		ap_var_t var = ap_environment_var_of_dim(lcenv,(ap_dim_t)i);
		if (!ap_environment_mem_var(env1,var) || !ap_environment_mem_var(env2,var)) {
			//size_t size = intersect->intdim + intersect->realdim;
			intersect = ap_environment_remove(intersect,&var,1);
		}	
	}	
	return intersect;
}

/// common_environment = modifies exp1 and exp2 such that 
/// they have the same common environment
///
void common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2) {

	// we compute the least common environment for the two expressions
	ap_environment_t* lcenv = common_environment(
			(*exp1)->env,
			(*exp2)->env);
	// we extend the environments such that both expressions have the same one
	*exp1 = ap_texpr1_extend_environment(*exp1,lcenv);
	*exp2 = ap_texpr1_extend_environment(*exp2,lcenv);
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
		if (val->getType()->getPrimitiveSizeInBits() == 1) {
			// actually, this is a boolean variable
			return 1;
		}
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
		);
		ap_type = AP_RTYPE_REAL;
		return 1;
	}
	return 0;
}

void environment_print(ap_environment_t * env) {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR: tmpfile has not been created\n";
		return;
	}

	ap_environment_fdump(tmp,env);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}

void texpr1_print(ap_texpr1_t * expr) {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR: tmpfile has not been created\n";
		return;
	}

	ap_texpr1_fprint(tmp,expr);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}

void tcons1_array_print(ap_tcons1_array_t * cons) {

	FILE* tmp = tmpfile();
	if (tmp == NULL) {
		*Out << "ERROR: tmpfile has not been created\n";
		return;
	}

	ap_tcons1_array_fprint(tmp,cons);
	fseek(tmp,0,SEEK_SET);
	char c;
	while ((c = (char)fgetc(tmp))!= EOF)
		*Out << c;
	fclose(tmp);
}
