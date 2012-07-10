#include <map>

#include "llvm/Support/CFG.h"
#include "llvm/Constants.h"

#include "Expr.h"
#include "apron.h"
#include "Debug.h"
#include "Analyzer.h"
#include "AIpass.h"
#include "Live.h"

std::map<Value*, ap_texpr1_t*> Exprs;
std::map<ap_var_t, ap_texpr1_t*> Exprs_var;

void Expr::set_expr(Value * val, Expr exp) {
	if (Exprs.count(val))
		ap_texpr1_free(Exprs[val]);
	Exprs[val] = ap_texpr1_copy(exp.ap_expr);
}

void Expr::clear_exprs() {
	Exprs.clear();
	Exprs_var.clear();
}

Expr::Expr(Value * val) {
	ap_expr = NULL;

	if (Exprs.count(val)) {
		ap_expr = ap_texpr1_copy(Exprs[val]);
		return;
	}

	if (isa<Constant>(val)) {
		ap_expr = create_ap_expr(dyn_cast<Constant>(val));
		return;
	}

	if (Instruction * I = dyn_cast<Instruction>(val)) {
		ap_expr = visit(*I);
		return;
	}
}

Expr::Expr(ap_var_t var) {
	if (Exprs_var.count(var)) {
		ap_expr = ap_texpr1_copy(Exprs_var[var]);
	} else {
		ap_expr = create_ap_expr(var);
		Exprs_var[var] = ap_expr;
		//Exprs[(Value*)var] = ap_texpr1_copy(ap_expr);
	}
}

Expr::Expr(ap_texpr_op_t op, Expr exp1, Expr exp2, ap_texpr_rtype_t type, ap_texpr_rdir_t round) {
	ap_expr = ap_texpr1_binop(op,
			ap_texpr1_copy(exp1.ap_expr),
			ap_texpr1_copy(exp2.ap_expr),
			type,
			round);
}

Expr::~Expr() {
	ap_texpr1_free(ap_expr);
}

Expr::Expr(const Expr &exp) {
	ap_expr = ap_texpr1_copy(exp.ap_expr);
}

Expr & Expr::operator= (const Expr & exp) {
	ap_texpr1_free(ap_expr);
	ap_expr = ap_texpr1_copy(exp.ap_expr);
}

ap_texpr1_t * Expr::getExpr() {
	return ap_expr;
}

ap_environment_t * Expr::getEnv() {
	return ap_expr->env;
}

void Expr::print() {
	texpr1_print(ap_expr);
}

ap_texpr1_t * Expr::create_ap_expr(Constant * val) {
	ap_texpr1_t * res = NULL;
	if (isa<ConstantInt>(val)) {
		ConstantInt * Int = dyn_cast<ConstantInt>(val);
		// it is supposed we use signed int
		int64_t i = Int->getSExtValue();
		res = ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),i);
	} 
	if (isa<ConstantFP>(val)) {
		ConstantFP * FP = dyn_cast<ConstantFP>(val);
		double x = FP->getValueAPF().convertToDouble();
		if (!FP->isExactlyValue(x)) {
			float f = FP->getValueAPF().convertToFloat(); 
			x = f;
		}
		res = ap_texpr1_cst_scalar_double(ap_environment_alloc_empty(),x);
	}
	if (isa<ConstantPointerNull>(val)) {
		res = ap_texpr1_cst_scalar_int(ap_environment_alloc_empty(),0);
	}
	if (isa<UndefValue>(val)) {
		res = create_ap_expr((ap_var_t)val);
	}
	if (res == NULL)
		res = create_ap_expr((ap_var_t)val);
	return res;
}

ap_texpr1_t * Expr::create_ap_expr(ap_var_t var) {
	ap_environment_t* env = NULL;
	ap_texpr_rtype_t ap_type;

	if (get_ap_type((Value*)var, ap_type)) return NULL;

	if (ap_type == AP_RTYPE_INT) { 
		env = ap_environment_alloc(&var,1,NULL,0);
	} else {
		env = ap_environment_alloc(NULL,0,&var,1);
	}
	return ap_texpr1_var(env,var);
}

void Expr::create_constraints (
	ap_constyp_t constyp,
	Expr expr,
	Expr nexpr,
	std::vector<ap_tcons1_array_t*> * t_cons
	) {
	
	ap_tcons1_t cons;
	ap_tcons1_array_t * consarray;
	
	if (constyp == AP_CONS_DISEQ) {
		// we have a disequality constraint. We tranform it into 2 different
		// constraints: < and >, in order to create further 2 abstract domain
		// instead of one.
		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				AP_CONS_SUP,
				ap_texpr1_copy(expr.ap_expr),
				ap_scalar_alloc_set_double(0));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		t_cons->push_back(consarray);

		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				AP_CONS_SUP,
				ap_texpr1_copy(nexpr.ap_expr),
				ap_scalar_alloc_set_double(0));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		t_cons->push_back(consarray);
	} else {
		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				constyp,
				ap_texpr1_copy(expr.ap_expr),
				ap_scalar_alloc_set_double(0));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		t_cons->push_back(consarray);
	}
}

/// common_environment = modifies exp1 and exp2 such that 
/// they have the same common environment
///
void Expr::common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2) {

	// we compute the least common environment for the two expressions
	ap_environment_t* lcenv = common_environment(
			(*exp1)->env,
			(*exp2)->env);
	// we extend the environments such that both expressions have the same one
	*exp1 = ap_texpr1_extend_environment(*exp1,lcenv);
	*exp2 = ap_texpr1_extend_environment(*exp2,lcenv);
	ap_environment_free(lcenv);
}

ap_environment_t * Expr::common_environment(
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

void Expr::common_environment(Expr* exp1, Expr* exp2) {
	ap_texpr1_t * texpr1 = exp1->ap_expr;
	ap_texpr1_t * texpr2 = exp2->ap_expr;
	common_environment(&texpr1,&texpr2);
	exp1->ap_expr = texpr1;
	exp2->ap_expr = texpr2;
}

ap_environment_t * Expr::intersect_environment(
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

int Expr::get_ap_type(Value * val,ap_texpr_rtype_t &ap_type) {

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
			// unknown type
			ap_type = AP_RTYPE_REAL;
			return 1;
	}
	return 0;
}

void Expr::environment_print(ap_environment_t * env) {

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

void Expr::texpr1_print(ap_texpr1_t * expr) {

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

void Expr::tcons1_array_print(ap_tcons1_array_t * cons) {

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

ap_texpr1_t * Expr::visitReturnInst (ReturnInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitBranchInst (BranchInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSwitchInst (SwitchInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitIndirectBrInst (IndirectBrInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInvokeInst (InvokeInst &I){
	return visitInstAndAddVar(I);
}

#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MINOR == 0
ap_texpr1_t * Expr::visitUnwindInst (UnwindInst &I){
	return visitInstAndAddVar(I);
}
#endif

ap_texpr1_t * Expr::visitUnreachableInst (UnreachableInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitICmpInst (ICmpInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFCmpInst (FCmpInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitAllocaInst (AllocaInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitLoadInst (LoadInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitStoreInst (StoreInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitGetElementPtrInst (GetElementPtrInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitPHINode (PHINode &I){
	Value * pv;
	// if the PHINode has actually one single incoming edge, we just say the
	// value is equal to its associated expression
	// There is no need to introduce PHIvars...
	if (I.getNumIncomingValues() == 1) {
		pv = I.getIncomingValue(0);
		ap_texpr1_t * res;
		if (isa<UndefValue>(pv)) {
			res = create_ap_expr(dyn_cast<Constant>(pv));
		}
		if (isa<Constant>(pv)) {
			res = create_ap_expr(dyn_cast<Constant>(pv));
		}

		if (Instruction * inst = dyn_cast<Instruction>(pv)) {
			res = visit(inst);
		}
		return res;
	} else {
		return visitInstAndAddVar(I);
	}
}

ap_texpr1_t * Expr::visitTruncInst (TruncInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitZExtInst (ZExtInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSExtInst (SExtInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPTruncInst (FPTruncInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPExtInst (FPExtInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPToUIInst (FPToUIInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitFPToSIInst (FPToSIInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitUIToFPInst (UIToFPInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSIToFPInst (SIToFPInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitPtrToIntInst (PtrToIntInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitIntToPtrInst (IntToPtrInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitBitCastInst (BitCastInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitSelectInst (SelectInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitCallInst(CallInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitVAArgInst (VAArgInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitExtractElementInst (ExtractElementInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInsertElementInst (InsertElementInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitShuffleVectorInst (ShuffleVectorInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitExtractValueInst (ExtractValueInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInsertValueInst (InsertValueInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitTerminatorInst (TerminatorInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitBinaryOperator (BinaryOperator &I){
	ap_texpr_op_t op;
	switch(I.getOpcode()) {
		// Standard binary operators...
		case Instruction::Add : 
		case Instruction::FAdd: 
			op = AP_TEXPR_ADD;
			break;
		case Instruction::Sub : 
		case Instruction::FSub: 
			op = AP_TEXPR_SUB;
			break;
		case Instruction::Mul : 
		case Instruction::FMul: 
			op = AP_TEXPR_MUL;
			break;
		case Instruction::UDiv: 
		case Instruction::SDiv: 
		case Instruction::FDiv: 
			op = AP_TEXPR_DIV;
			break;
		case Instruction::URem: 
		case Instruction::SRem: 
		case Instruction::FRem: 
			op = AP_TEXPR_MOD;
			break;
			// Logical operators
		case Instruction::Shl : // Shift left  (logical)
		case Instruction::LShr: // Shift right (logical)
		case Instruction::AShr: // Shift right (arithmetic)
		case Instruction::And :
		case Instruction::Or  :
		case Instruction::Xor :
		case Instruction::BinaryOpsEnd:
			// we consider the result is unknown
			// so we create a new variable
			return visitInstAndAddVar(I);
	}
	ap_texpr_rtype_t type;
	get_ap_type(&I,type);
	ap_texpr_rdir_t dir = AP_RDIR_RND;
	Value * op1 = I.getOperand(0);
	Value * op2 = I.getOperand(1);

	Expr exp1(op1);
	Expr exp2(op2);
	common_environment(&exp1,&exp2);

	// we create the expression associated to the binary op
	ap_texpr1_t * exp = ap_texpr1_binop(op, ap_texpr1_copy(exp1.ap_expr), ap_texpr1_copy(exp2.ap_expr), type, dir);
	return exp;
}

ap_texpr1_t * Expr::visitCmpInst (CmpInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitCastInst (CastInst &I){
	return visitInstAndAddVar(I);
}

ap_texpr1_t * Expr::visitInstAndAddVar(Instruction &I) {
	ap_environment_t* env = NULL;
	ap_var_t var = (Value *) &I; 
	ap_texpr_rtype_t ap_type;

	if (get_ap_type((Value*)&I, ap_type)) return NULL;

	if (ap_type == AP_RTYPE_INT) { 
		env = ap_environment_alloc(&var,1,NULL,0);
	} else {
		env = ap_environment_alloc(NULL,0,&var,1);
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	return exp;
}
