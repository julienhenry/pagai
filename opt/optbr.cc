#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>


#include "optbr.h"

using namespace llvm;

void Optbr::visitBranchInst(BranchInst &I) {
	if (I.isConditional()) {
		BasicBlock * succ = I.getSuccessor(0);
		for (unsigned i = 1; i < I.getNumSuccessors(); i++) {
			if (succ != I.getSuccessor(i))
				return;
		}
		// conditionnal branch that always goes to the same successor
		// one can transform this branch into an unconditional one
		BasicBlock * parent = I.getParent();
		I.eraseFromParent();
		//BranchInst * newbr = BranchInst::Create(succ,parent);

		for (BasicBlock::iterator i = succ->begin(), e = succ->end(); i != e; ++i) {
			if (PHINode * phi = dyn_cast<PHINode>(i)) {
restart:
				unsigned seen = 0;
				for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
					if (phi->getIncomingBlock(i) == parent) {
						if (seen > 0) {
							phi->removeIncomingValue(i);
							goto restart;
						}
						seen++;
					}
				}
				llvm::outs() << *phi << "\n";
			}
		}
	}
}

bool Optbr::runOnBasicBlock(BasicBlock &BB) {
	visit(BB.getTerminator());
	return 0;
}

char Optbr::ID = 0;
static RegisterPass<Optbr> X("optbr", "Br optimisation Pass", false, false);
