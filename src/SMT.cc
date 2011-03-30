#include <sstream>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Analysis/LoopInfo.h"

#include "SMT.h"

#include "SMT_manager.h"
#include "z3_manager.h"
#include "yices.h"
#include "Expr.h"


char SMT::ID = 0;
static RegisterPass<SMT>
X("SMT","SMT-formula creation pass",false,true);


const char * SMT::getPassName() const {
	return "SMT";
}

SMT::SMT() : FunctionPass(ID) {
	man = new z3_manager();
}

SMT::~SMT() {
	delete man;
}

void SMT::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfo>();
	AU.setPreservesAll();
}

bool SMT::runOnFunction(Function &F) {
	LI = &(getAnalysis<LoopInfo>());
	return 0;
}


std::set<BasicBlock*>* SMT::getPr(Function &F) {
	if (!Pr.count(&F))
		computePr(F);
	return Pr[&F];
}

SMT_expr SMT::getRho(Function &F) {
	if (!rho.count(&F))
		computeRho(F);
	return rho[&F];
}

SMT_expr SMT::texpr1ToSmt(ap_texpr1_t texpr) {
	return NULL;
}

SMT_expr SMT::linexpr1ToSmt(ap_linexpr1_t linexpr, bool &integer) {
	std::vector<SMT_expr> elts;
	SMT_expr val;
	SMT_expr coefficient;

	integer = false;
	bool iszero;
	size_t i;
	ap_var_t var;
	ap_coeff_t* coeff;

	ap_linexpr1_ForeachLinterm1(&linexpr,i,var,coeff){ 
		val = getValueExpr((Value*)var);
		if (((Value*)var)->getType()->isIntegerTy()) {
			coefficient = scalarToSmt(coeff->val.scalar,true,iszero);
			if (!iszero) integer = true;
		} else {
			coefficient = scalarToSmt(coeff->val.scalar,false,iszero);
		}
		if (coefficient) 
			printf("coefficient != NULL\n");
		else
			printf("coefficient = NULL\n");

		if (!iszero) {
			std::vector<SMT_expr> elt;
			elt.push_back(val);
			elt.push_back(coefficient);
			elts.push_back(man->SMT_mk_mul(elt));
		}
	}
	coeff = ap_linexpr1_cstref(&linexpr);
	coefficient = scalarToSmt(coeff->val.scalar,integer,iszero);
	elts.push_back(coefficient);
	printf("OK\n");
	return man->SMT_mk_sum(elts);
}

SMT_expr SMT::scalarToSmt(ap_scalar_t * scalar, bool integer, bool &iszero) {
	double val;
	mp_rnd_t round = GMP_RNDN;
	ap_double_set_scalar(&val,scalar,round);
	
	if (val == 0) iszero = true;
	else iszero = false;
	printf("val = %d\n",val);
	if (integer)
		return man->SMT_mk_num((int)val);
	else
		return man->SMT_mk_real(val);
}

SMT_expr SMT::lincons1ToSmt(ap_lincons1_t lincons) {
	ap_constyp_t* constyp = ap_lincons1_constypref(&lincons);
	ap_linexpr1_t linexpr = ap_lincons1_linexpr1ref(&lincons);
	ap_coeff_t * coeff = ap_lincons1_cstref(&lincons);
	SMT_expr scalar_smt = NULL;
	bool integer;
	SMT_expr linexpr_smt = linexpr1ToSmt(linexpr, integer);
	if (integer)
		scalar_smt = man->SMT_mk_num(0);
	else
		scalar_smt = man->SMT_mk_real(0);

	switch (*constyp) {
	case AP_CONS_EQ:
		return man->SMT_mk_eq(linexpr_smt,scalar_smt);
	case AP_CONS_SUPEQ:
		return man->SMT_mk_ge(linexpr_smt,scalar_smt);
	case AP_CONS_SUP: 
		return man->SMT_mk_gt(linexpr_smt,scalar_smt);
	case AP_CONS_EQMOD:
		return man->SMT_mk_eq(linexpr_smt,scalar_smt);
	case AP_CONS_DISEQ:
		return man->SMT_mk_diseq(linexpr_smt,scalar_smt);
	}
	// unreachable
	return NULL;
}

SMT_expr SMT::tcons1ToSmt(ap_tcons1_t tcons) {
	ap_constyp_t* constyp = ap_tcons1_constypref(&tcons);
	ap_texpr1_t texpr = ap_tcons1_texpr1ref(&tcons);
	ap_scalar_t* scalar = ap_tcons1_scalarref(&tcons);
	bool integer = true;
	bool iszero;
	SMT_expr texpr_smt = texpr1ToSmt(texpr);
	SMT_expr scalar_smt = scalarToSmt(scalar,integer,iszero);

	switch (*constyp) {
	case AP_CONS_EQ:
		return man->SMT_mk_eq(texpr_smt,scalar_smt);
	case AP_CONS_SUPEQ:
		return man->SMT_mk_ge(texpr_smt,scalar_smt);
	case AP_CONS_SUP: 
		return man->SMT_mk_gt(texpr_smt,scalar_smt);
	case AP_CONS_EQMOD:
		return man->SMT_mk_eq(texpr_smt,scalar_smt);
	case AP_CONS_DISEQ:
		return man->SMT_mk_diseq(texpr_smt,scalar_smt);
	}
	// unreachable
	return NULL;
}

SMT_expr SMT::AbstractToSmt(Abstract * A) {
	std::vector<SMT_expr> constraints;
	ap_lincons1_t lincons;
	ap_lincons1_array_t lincons_array = A->to_lincons_array();
	size_t n = ap_lincons1_array_size(&lincons_array);
	for (size_t i = 0; i < n; i++) {
		lincons = ap_lincons1_array_get(&lincons_array,i);
		constraints.push_back(lincons1ToSmt(lincons));
	}
	if (constraints.size() == 0)
		return man->SMT_mk_true();
	else
		return man->SMT_mk_and(constraints);
}


std::string SMT::getNodeName(BasicBlock* b, bool src) {
	std::ostringstream name;
	std::set<BasicBlock*> * FPr = getPr(*(b->getParent()));
	if (FPr->count(b)) {
		if (src)
			name << "bs_";
		else
			name << "bd_";
	} else {
		name << "b_";
	}
	name << b;
	return name.str();
}

std::string SMT::getEdgeName(BasicBlock* b1, BasicBlock* b2) {
	std::ostringstream name;
	name << "e_" << b1 << "_" << b2;
	return name.str();
}

std::string SMT::getValueName(Value * v) {
	std::ostringstream name;
	name << "x_" << v;
	return name.str();
}


SMT_expr SMT::getValueType(Value * v) {
	switch (v->getType()->getTypeID()) {
		case Type::IntegerTyID:
			return man->int_type;
		default:
			return man->float_type;
	}
}

SMT_var SMT::getVar(Value * v) {
	if (!vars.count(v))
		vars[v] = man->SMT_mk_var(getValueName(v),getValueType(v));
	return vars[v];
}

SMT_expr SMT::getValueExpr(Value * v) {
	
	ap_texpr_rtype_t ap_type;
	if (get_ap_type(v, ap_type)) return NULL;

	if (isa<Constant>(v)) {
		if (isa<ConstantInt>(v)) { 
			ConstantInt * Int = dyn_cast<ConstantInt>(v);
			int64_t n = Int->getSExtValue();
			return man->SMT_mk_num((int)n);
		} 
		if (isa<ConstantFP>(v)) {
			ConstantFP * FP = dyn_cast<ConstantFP>(v);
			double x = FP->getValueAPF().convertToDouble();
			return man->SMT_mk_real(x);
		}
		if (isa<UndefValue>(v)) {
			SMT_var var = getVar(v);
			return man->SMT_mk_expr_from_var(var);
		}
	} else if (isa<Instruction>(v) || isa<Argument>(v)) {
		SMT_var var = getVar(v);
		return man->SMT_mk_expr_from_var(var);
	} else {
		fouts() << "ERROR in getValueExpr" << *v << "\n";
		fouts().flush();
		return NULL;
	}
	return NULL;
}

// computePr - computes the set Pr of BasicBlocks
// for the moment - Pr = Pw
void SMT::computePr(Function &F) {
	std::set<BasicBlock*> * FPr = new std::set<BasicBlock*>();
	BasicBlock * b;

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		b = i;
		if (LI->isLoopHeader(b)) {
			FPr->insert(b);
		}
	}
	Pr[&F] = FPr;
}

void SMT::computeRho(Function &F) {
	BasicBlock * b;
	
	printf("computing Rho\n");
	fflush(stdout);
	rho_components.clear();

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		b = i;
		// firstly, we create a boolean reachability predicate for the basicblock
		SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,false));
		// we associate it the right predicate depending on its incoming edges
		std::vector<SMT_expr> predicate;
		for (pred_iterator p = pred_begin(b), e = pred_end(b); p != e; ++p) {
			SMT_var evar = man->SMT_mk_bool_var(getEdgeName(*p,b));
			predicate.push_back(man->SMT_mk_expr_from_bool_var(evar));
		}
		SMT_expr bpredicate;
		
		bpredicate = man->SMT_mk_or(predicate);

		if (bpredicate != NULL) {
			SMT_expr bvar_exp = man->SMT_mk_expr_from_bool_var(bvar);
			bvar_exp = man->SMT_mk_eq(bvar_exp,bpredicate);
			rho_components.push_back(bvar_exp);
		}
		// we compute the transformation due to the basicblock's
		// instructions
		bvar = man->SMT_mk_bool_var(getNodeName(b,true));
		instructions.clear();
		for (BasicBlock::iterator i = b->begin(), e = b->end();
				i != e; ++i) {
			visit(*i);
		}
		bpredicate = man->SMT_mk_or(instructions);
		if (bpredicate != NULL) {
			SMT_expr bvar_exp = man->SMT_mk_expr_from_bool_var(bvar);
			bvar_exp = man->SMT_mk_not(bvar_exp);
			std::vector<SMT_expr> implies;
			implies.push_back(bvar_exp);
			implies.push_back(bpredicate);
			bvar_exp = man->SMT_mk_or(implies);
			rho_components.push_back(bvar_exp);
		}
	}

	rho[&F] = man->SMT_mk_and(rho_components); 
	printf("Rho computation finished ...\n %d\n", rho[&F]);
	fflush(stdout);
	man->SMT_print(rho[&F]);
	printf("Rho printing finished ...\n");
	fflush(stdout);
}

void SMT::visitReturnInst (ReturnInst &I) {
}

SMT_expr SMT::computeCondition(PHINode * inst) {
	
	//SMT_expr expr = getValueExpr(inst);	
	return construct_phi_ite(*inst,0,inst->getNumIncomingValues());

}

SMT_expr SMT::computeCondition(CmpInst * inst) {

	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)inst->getOperand(0), ap_type)) return NULL;

	SMT_expr op1 = getValueExpr(inst->getOperand(0));
	SMT_expr op2 = getValueExpr(inst->getOperand(1));

	switch (inst->getPredicate()) {
		case CmpInst::FCMP_FALSE:
			return man->SMT_mk_false();
		case CmpInst::FCMP_TRUE: 
			return man->SMT_mk_true();
		case CmpInst::FCMP_OEQ: 
		case CmpInst::FCMP_UEQ: 
		case CmpInst::ICMP_EQ:
			return man->SMT_mk_eq(op1,op2);
		case CmpInst::FCMP_OGT:
		case CmpInst::FCMP_UGT:
		case CmpInst::ICMP_UGT: 
		case CmpInst::ICMP_SGT: 
			return man->SMT_mk_gt(op1,op2);
		case CmpInst::FCMP_OLT: 
		case CmpInst::FCMP_ULT: 
		case CmpInst::ICMP_ULT: 
		case CmpInst::ICMP_SLT: 
			return man->SMT_mk_lt(op1,op2);
		case CmpInst::FCMP_OGE:
		case CmpInst::FCMP_UGE:
		case CmpInst::ICMP_UGE:
		case CmpInst::ICMP_SGE:
			return man->SMT_mk_ge(op1,op2);
		case CmpInst::FCMP_OLE:
		case CmpInst::FCMP_ULE:
		case CmpInst::ICMP_ULE:
		case CmpInst::ICMP_SLE:
			return man->SMT_mk_le(op1,op2);
		case CmpInst::FCMP_ONE:
		case CmpInst::FCMP_UNE: 
		case CmpInst::ICMP_NE: 
			return man->SMT_mk_diseq(op1,op2);
		case CmpInst::FCMP_ORD: 
		case CmpInst::FCMP_UNO:
		case CmpInst::BAD_ICMP_PREDICATE:
		case CmpInst::BAD_FCMP_PREDICATE:
			fouts() << "ERROR : Unknown predicate\n";
			return NULL;
	}
	return NULL;
}

void SMT::visitBranchInst (BranchInst &I) {
	BasicBlock * b = I.getParent();

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,true));
	SMT_expr bexpr = man->SMT_mk_expr_from_bool_var(bvar);
	BasicBlock * s = I.getSuccessor(0);
	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	SMT_expr eexpr = man->SMT_mk_expr_from_bool_var(evar);
	SMT_expr components_and;

	if (I.isUnconditional()) {
		rho_components.push_back(man->SMT_mk_eq(eexpr,bexpr));
	} else {
		std::vector<SMT_expr> components;
		SMT_expr cond;
		if (isa<CmpInst>(I.getOperand(0)))
			cond = computeCondition(dyn_cast<CmpInst>(I.getOperand(0)));
		else if (isa<PHINode>(I.getOperand(0))) {
			return;
			// NOT IMPLEMENTED !!
			cond = computeCondition(dyn_cast<PHINode>(I.getOperand(0)));
		} else
			cond = NULL;

		components.push_back(bexpr);
		if (cond != NULL)
			components.push_back(cond);
		components_and = man->SMT_mk_and(components);
		rho_components.push_back(man->SMT_mk_eq(eexpr,components_and));

		components.clear();
		cond = man->SMT_mk_not(cond);
		s = I.getSuccessor(1);
		evar = man->SMT_mk_bool_var(getEdgeName(b,s));
		eexpr = man->SMT_mk_expr_from_bool_var(evar);
		components.push_back(bexpr);
		if (cond != NULL)
			components.push_back(cond);
		components_and = man->SMT_mk_and(components);
		rho_components.push_back(man->SMT_mk_eq(eexpr,components_and));
	}
}

void SMT::visitSwitchInst (SwitchInst &I) {
}

void SMT::visitIndirectBrInst (IndirectBrInst &I) {
}

void SMT::visitInvokeInst (InvokeInst &I) {
}

void SMT::visitUnwindInst (UnwindInst &I) {
}

void SMT::visitUnreachableInst (UnreachableInst &I) {
}

void SMT::visitICmpInst (ICmpInst &I) {
}

void SMT::visitFCmpInst (FCmpInst &I) {
}

void SMT::visitAllocaInst (AllocaInst &I) {
}

void SMT::visitLoadInst (LoadInst &I) {
}

void SMT::visitStoreInst (StoreInst &I) {
}

void SMT::visitGetElementPtrInst (GetElementPtrInst &I) {
}

SMT_expr SMT::construct_phi_ite(PHINode &I, unsigned i, unsigned n) {
	if (i == n-1) {
		// this is the last possible value of the PHI-variable
		return getValueExpr(I.getIncomingValue(i));
	}
	SMT_expr incomingVal = 	getValueExpr(I.getIncomingValue(i));

	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(I.getIncomingBlock(i),I.getParent()));
	SMT_expr incomingBlock = man->SMT_mk_expr_from_bool_var(evar);

	SMT_expr tail = construct_phi_ite(I,i+1,n);
	return man->SMT_mk_ite(incomingBlock,incomingVal,tail);
}

void SMT::visitPHINode (PHINode &I) {
	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)&I, ap_type)) return;

	SMT_expr expr = getValueExpr(&I);	
	SMT_expr assign = construct_phi_ite(I,0,I.getNumIncomingValues());

	instructions.push_back(man->SMT_mk_eq(expr,assign));
}

void SMT::visitTruncInst (TruncInst &I) {
}

void SMT::visitZExtInst (ZExtInst &I) {
}

void SMT::visitSExtInst (SExtInst &I) {
}

void SMT::visitFPTruncInst (FPTruncInst &I) {
}

void SMT::visitFPExtInst (FPExtInst &I) {
}

void SMT::visitFPToUIInst (FPToUIInst &I) {
}

void SMT::visitFPToSIInst (FPToSIInst &I) {
}

void SMT::visitUIToFPInst (UIToFPInst &I) {
}

void SMT::visitSIToFPInst (SIToFPInst &I) {
}

void SMT::visitPtrToIntInst (PtrToIntInst &I) {
}

void SMT::visitIntToPtrInst (IntToPtrInst &I) {
}

void SMT::visitBitCastInst (BitCastInst &I) {
}

void SMT::visitSelectInst (SelectInst &I) {
}

void SMT::visitCallInst(CallInst &I) {
}

void SMT::visitVAArgInst (VAArgInst &I) {
}

void SMT::visitExtractElementInst (ExtractElementInst &I) {
}

void SMT::visitInsertElementInst (InsertElementInst &I) {
}

void SMT::visitShuffleVectorInst (ShuffleVectorInst &I) {
}

void SMT::visitExtractValueInst (ExtractValueInst &I) {
}

void SMT::visitInsertValueInst (InsertValueInst &I) {
}

void SMT::visitTerminatorInst (TerminatorInst &I) {
}

void SMT::visitBinaryOperator (BinaryOperator &I) {
	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)&I, ap_type)) return;

	SMT_expr expr = getValueExpr(&I);	
	SMT_expr assign = NULL;	
	std::vector<SMT_expr> operands;
	operands.push_back(getValueExpr(I.getOperand(0)));
	operands.push_back(getValueExpr(I.getOperand(1)));
	switch(I.getOpcode()) {
		// Standard binary operators...
		case Instruction::Add : 
		case Instruction::FAdd: 
			assign = man->SMT_mk_sum(operands);
			break;
		case Instruction::Sub : 
		case Instruction::FSub: 
			assign = man->SMT_mk_sub(operands);
			break;
		case Instruction::Mul : 
		case Instruction::FMul: 
			assign = man->SMT_mk_mul(operands);
			break;
		case Instruction::And :
			assign = man->SMT_mk_and(operands);
			break;
		case Instruction::Or  :
			assign = man->SMT_mk_or(operands);
			break;
			// the others are not implemented
		case Instruction::Xor :
		case Instruction::Shl : // Shift left  (logical)
		case Instruction::LShr: // Shift right (logical)
		case Instruction::AShr: // Shift right (arithmetic)
		case Instruction::BinaryOpsEnd:
		case Instruction::UDiv: 
		case Instruction::SDiv: 
		case Instruction::FDiv: 
		case Instruction::URem: 
		case Instruction::SRem: 
		case Instruction::FRem: 
			// NOT IMPLEMENTED
			return;
	}
	instructions.push_back(man->SMT_mk_eq(expr,assign));
}

void SMT::visitCmpInst (CmpInst &I) {
}

void SMT::visitCastInst (CastInst &I) {
}
