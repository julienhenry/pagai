#ifndef _INITVERIF_H
#define _INITVERIF_H

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LiveValues.h"

using namespace llvm;



class initVerif : public ModulePass {
	
public:
	static char ID;	

	initVerif () : ModulePass(ID) {}
	~initVerif () {}
		
	const char *getPassName() const;
	void getAnalysisUsage(AnalysisUsage &AU) const;
	bool runOnModule(Module &M);
	
	void computeFunction(Function * F);
	void printBasicBlock(BasicBlock* b);
};

#endif
