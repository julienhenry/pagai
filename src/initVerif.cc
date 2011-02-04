#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/FormattedStream.h"

#include "initVerif.h"
#include "hashtables.h"

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
	/*we find the loop heads*/
	if (F->size() > 0) {
		node * front = nodes[&(F->front())];
		front->compute_loop_heads();
	}

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		computeBasicBlock(i);
	}
}




void initVerif::computeBasicBlock(BasicBlock* b) {
	node * n = nodes[b];
	if (n->getLoop()) {
		fouts() << b << ": LOOP HEAD" << *b;
	} else {
		fouts() << b << ":" << *b;
	}
}
