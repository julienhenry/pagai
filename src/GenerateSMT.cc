
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Analysis/LoopInfo.h"


#include "Analyzer.h"
#include "SMT.h"
#include "Node.h"
#include "SMT_manager.h"
#include "z3_manager.h"
#include "yices.h"
#include "Debug.h"
#include "GenerateSMT.h"

using namespace std;

char GenerateSMT::ID = 0;
static RegisterPass<GenerateSMT>
X("GenerateSMT","SMT-lib formula generation pass",false,true);


const char * GenerateSMT::getPassName() const {
	return "SMT-Lib Generation Pass";
}


GenerateSMT::GenerateSMT() : ModulePass(ID) {

}

GenerateSMT::~GenerateSMT() {

}

void GenerateSMT::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfo>();
	AU.addRequired<Live>();
	AU.addRequired<SMT>();
	AU.setPreservesAll();
}

bool GenerateSMT::runOnFunction(Function &F) {
	LSMT->getPr(F);
	LSMT->getRho(F);

	LSMT->man->SMT_print(LSMT->getRho(F));

	*Out << "\n\n-------\n\n";

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		BasicBlock * b = i;
		printBasicBlock(b);
	}
	return 0;
}

void GenerateSMT::printBasicBlock(BasicBlock* b) {
	int N = 0;
	for (BasicBlock::iterator i = b->begin(), e = b->end(); i != e; ++i) {
		N++;
	}
	*Out << "BasicBlock " << b  << ": " << N << " instruction(s)" << *b << "\n";
}

bool GenerateSMT::runOnModule(Module &M) {
	LSMT = &(getAnalysis<SMT>());
	Function * F;

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		if (F->isDeclaration()) continue;
		runOnFunction(*F);
	}

	return 0;
}
