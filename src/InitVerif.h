#ifndef _INITVERIF_H
#define _INITVERIF_H

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;



class initVerif : public ModulePass {
	
public:
	static char ID;	

	initVerif () : ModulePass(ID) {}
	~initVerif () {}
		
	const char *getPassName() const;
	bool runOnModule(Module &M);
	
	void computeFunction(Function * F);
	void printBasicBlock(BasicBlock* b);
};

#endif
