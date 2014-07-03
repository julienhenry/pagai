#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"


using namespace llvm;

class GlobalToLocal : public ModulePass {
 public:
  static char ID;
  GlobalToLocal() : ModulePass(ID) {}

  bool runOnModule(Module &M);
	
  bool hasOnlyOneFunction(Module &M);
};

