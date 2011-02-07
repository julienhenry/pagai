#include<stack>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/FormattedStream.h"

#include "InitVerif.h"
#include "Hashtables.h"


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


bool initVerif::runOnModule(Module &M) {

	
	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function* F = &*mIt;
		fouts() << "1 function found, of size " << F->size() << "\n";	
		computeFunction(F);
	}
	return 0;
}

void initVerif::computeFunction(Function * F) {
	node * n;
	
	/*we create the node objects associated to each basicblock*/
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		n = new node(i);
		nodes[i] = n;
	}
	if (F->size() > 0) {
		/*we find the loop heads and the Strongly Connected Components*/
		node * front = nodes[&(F->front())];
		front->computeSCC();
	}
	DEBUG_MODE(
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
		printBasicBlock(i);
	)
}




void initVerif::printBasicBlock(BasicBlock* b) {
	node * n = nodes[b];
	if (n->getLoop()) {
		fouts() << b << ": SCC=" << n->getSccId() << ": LOOP HEAD" << *b;
	} else {
		fouts() << b << ": SCC=" << n->getSccId() << ":" << *b;
	}
}
