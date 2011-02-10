#include<stack>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LiveValues.h"

#include "InitVerif.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"

#ifdef DEBUG
#define DEBUG_MODE(x) x 
#else
#define DEBUG_MODE(x)
#endif

using namespace llvm;

char initVerif::ID = 0;

const char *initVerif::getPassName() const {
	return "initVerif";
}

void initVerif::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LiveValues>();
}



bool initVerif::runOnModule(Module &M) {

	init_apron();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function* F = &*mIt;
		fouts() << "1 function found, of size " << F->size() << "\n";	
		computeFunction(F);
	}
	return 0;
}

void initVerif::computeFunction(Function * F) {
	Node * n;
	
	/*we create the Node objects associated to each basicblock*/
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		n = new Node(i);
		Nodes[i] = n;
	}
	if (F->size() > 0) {
		/*we find the loop heads and the Strongly Connected Components*/
		Node * front = Nodes[&(F->front())];
		front->computeSCC();
	}
	DEBUG_MODE(
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
		printBasicBlock(i);
	)
}




void initVerif::printBasicBlock(BasicBlock* b) {
	Node * n = Nodes[b];
	if (n->getLoop()) {
		fdbgs() << b << ": SCC=" << n->getSccId() << ": LOOP HEAD" << *b;
	} else {
		fdbgs() << b << ": SCC=" << n->getSccId() << ":" << *b;
	}
}
