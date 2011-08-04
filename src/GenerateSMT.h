#ifndef GENERATE_SMT_H
#define GENERATE_SMT_H


#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LoopInfo.h"

#include "SMT_manager.h"
#include "SMT.h"

using namespace llvm;


class GenerateSMT : public ModulePass {
	
	private:
		SMT * LSMT;

	public:
		static char ID;

		GenerateSMT();
		~GenerateSMT();

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);
		bool runOnFunction(Function &F);

		void printBasicBlock(BasicBlock* b);

};
#endif
