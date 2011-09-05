#include <vector>
#include <list>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/Analysis/Passes.h"

#include "ap_global1.h"
#include "pk.h"

#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMT.h"
#include "Debug.h"
#include "Analyzer.h"
#include "AIClassic.h"

using namespace llvm;

static RegisterPass<AIClassic> X("AIClassicPass", "Abstract Interpretation Pass", false, true);

char AIClassic::ID = 0;

const char * AIClassic::getPassName() const {
	return "AIClassic";
}

void AIClassic::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LoopInfo>();
	AU.addRequired<Live>();
	AU.addRequired<SMT>();
}

bool AIClassic::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = &(getAnalysis<SMT>());
	*Out << "Starting analysis: LW\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;

		Out->changeColor(raw_ostream::BLUE,true);
		*Out << "\n\n\n"
				<< "------------------------------------------\n"
				<< "-         COMPUTING FUNCTION             -\n"
				<< "------------------------------------------\n";
		Out->resetColor();
		LSMT->reset_SMTcontext();
		initFunction(F);
		computeFunction(F);

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (LSMT->getPr(*b->getParent())->count(b)) {
				Out->changeColor(raw_ostream::MAGENTA,true);
				*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
				Out->resetColor();
				n->X_s[passID]->print(true);
			}
			//delete Nodes[b];
		}
	}

	//*Out << "Number of iterations: " << n_iterations << "\n";
	//*Out << "Number of paths computed: " << n_paths << "\n";
	return 0;
}

void AIClassic::computeFunction(Function * F) {
	BasicBlock * b;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;

	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));
	LI = &(getAnalysis<LoopInfo>(*F));

	computeFunc(F,LSMT,LV,LI);
}
