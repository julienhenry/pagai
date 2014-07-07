#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/IRBuilder.h"

#include "expandequalities.h"

using namespace llvm;

bool ExpandEqualities::runOnFunction(Function &F) {

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		visit(i);
	}
	return 0;
}

void ExpandEqualities::visitBranchInst(BranchInst &I) {
	if (seen.count(&I)) return;
	seen.insert(&I);
	if (I.isUnconditional()) return;
	CmpInst * cond;
	if (!(cond = dyn_cast<CmpInst>(I.getCondition())))
		return;
	BasicBlock * eqblock;
	BasicBlock * neblock;
	Value * leftop = cond->getOperand(0);
	Value * rightop = cond->getOperand(1);
	bool isFloat = false;
	switch (cond->getPredicate()) {
		case CmpInst::FCMP_OEQ:
			isFloat = true;
		case CmpInst::ICMP_EQ:
			eqblock = I.getSuccessor(0);
			neblock = I.getSuccessor(1);
			break;
		case CmpInst::FCMP_ONE:
			isFloat = true;
		case CmpInst::ICMP_NE:
			eqblock = I.getSuccessor(1);
			neblock = I.getSuccessor(0);
			break;
		default:
			return;
	}
	// we create the structure:
	// if leftop < rightop then
	// ...
	// else if leftop > rightop then
	// ...
	IRBuilder<> Builder(I.getContext());
	Builder.SetInsertPoint(&I);
	CmpInst * cmpSLT;
	CmpInst * cmpSGT;
	if (isFloat) {
		cmpSLT = dyn_cast<CmpInst>(Builder.CreateFCmpOLT(leftop,rightop));
		cmpSGT = dyn_cast<CmpInst>(Builder.CreateFCmpOGT(leftop,rightop));
	} else {
		cmpSLT = dyn_cast<CmpInst>(Builder.CreateICmpSLT(leftop,rightop));
		cmpSGT = dyn_cast<CmpInst>(Builder.CreateICmpSGT(leftop,rightop));
	}
	llvm::SplitBlockAndInsertIfThen(
			cmpSLT, 
			false
			);
	llvm::SplitBlockAndInsertIfThen(
			cmpSGT, 
			false
			);
}

char ExpandEqualities::ID = 0;
static RegisterPass<ExpandEqualities> X("expandequalities", "Expand = and != to < and > ", false, false);
