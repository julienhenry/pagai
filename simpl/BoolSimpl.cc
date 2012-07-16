#include "BoolSimpl.h"
#include <iostream>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

void BoolSimpl::visitAnd(BinaryOperator &I) {
  Value *op0 = I.getOperand(0), *op1 = I.getOperand(1);
  if (CastInst *cop0 = dyn_cast<CastInst>(op0))
    if (CastInst *cop1 = dyn_cast<CastInst>(op1))
      if (cop0 -> isIntegerCast() &&
	  cop1 -> isIntegerCast() &&
	  cop0 -> getSrcTy() == cop1 -> getSrcTy()) {
	std::cerr << "replace AND" << std::endl;
	ReplaceInstWithInst(&I,
	  CastInst::CreateIntegerCast(
	    BinaryOperator::Create(BinaryOperator::And,
	      cop0 -> getOperand(0),
	      cop1 -> getOperand(0),
	      "and", &I),
	    cop0 -> getDestTy(),
            false,
            "postcast"));
      }
}

void BoolSimpl::visitICmpInst(ICmpInst &I) {
  // std::cerr << "cmp" << std::endl;
}

bool BoolSimpl::runOnBasicBlock(BasicBlock &bb) {
  visit(bb);
  return false;
}

char BoolSimpl::ID = 0;
static RegisterPass<BoolSimpl> X("BoolSimpl", "Simplify Boolean expressions", false, false);
