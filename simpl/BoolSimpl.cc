#include "BoolSimpl.h"
#include <iostream>

using namespace llvm;

void BoolSimpl::visitICmpInst(ICmpInst &I) {
  std::cerr << "cmp" << std::endl;
}

bool BoolSimpl::runOnBasicBlock(BasicBlock &bb) {
  visit(bb);
  return false;
}

char BoolSimpl::ID = 0;
static RegisterPass<BoolSimpl> X("BoolSimpl", "Simplify Boolean expressions", false, false);
