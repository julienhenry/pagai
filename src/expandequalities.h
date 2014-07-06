#ifndef EXPANDEQUALITIES_H
#define EXPANDEQUALITIES_H
#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include <set>


using namespace llvm;

class ExpandEqualities : public FunctionPass, 
	public InstVisitor<ExpandEqualities, void> {
	// make sure we do not expand twice the same branch inst
	std::set<Value*> seen;
 public:
  static char ID;
  ExpandEqualities() : FunctionPass(ID) {}

  bool runOnFunction(Function &F);

  void visitBranchInst(BranchInst &I);
	
};

#endif
