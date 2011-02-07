#ifndef _AI_H
#define _AI_H

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;



class AI : public ModulePass {
	
public:
	static char ID;	

	AI () : ModulePass(ID) {}
	~AI () {}
		
	const char *getPassName() const;
	bool runOnModule(Module &M);
	
	//void computeFunction(Function * F);
	//void computeBasicBlock(BasicBlock* b);
};

#endif
