#ifndef IDENTIFYLOOPS_H
#define IDENTIFYLOOPS_H
#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"


using namespace llvm;

extern std::set<BasicBlock *> Loop_headers;

class IdentifyLoops : public FunctionPass {
 public:
  static char ID;
  IdentifyLoops() : FunctionPass(ID) {}

  bool runOnFunction(Function &F);
		
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	
};

#endif

