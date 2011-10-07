#include "llvm/Support/CFG.h"

#include "GenerateSMT.h"
#include "Pr.h"
#include "Analyzer.h"
#include "Live.h"
#include "Node.h"
#include "Debug.h"

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
	AU.addRequired<Pr>();
	AU.addRequired<Live>();
	AU.addRequired<SMTpass>();
	AU.setPreservesAll();
}

bool GenerateSMT::runOnFunction(Function &F) {
	Pr::getPr(F);
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
	LSMT = &(getAnalysis<SMTpass>());
	Function * F;

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		if (F->isDeclaration()) continue;
		runOnFunction(*F);
	}

	return 0;
}
