#include "Live.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"
#include <stack>

using namespace llvm;

char Live::ID = 0;
static RegisterPass<Live>
X("live-values", "Value Liveness Analysis", false, true);


const char * Live::getPassName() const {
	return "Live";
}

Live::Live() : FunctionPass(ID) {}

void Live::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfo>();
	AU.setPreservesAll();
}

bool Live::runOnFunction(Function &F) {
	LI = &getAnalysis<LoopInfo>();

	// This pass' values are computed lazily, so there's nothing to do here.
	return false;
}

void Live::releaseMemory() {
	Memos.clear();
}

/// isUsedInBlock - Test if the given value is used in the given block.
///
bool Live::isUsedInBlock(Value *V, BasicBlock *BB) {
	Memo &M = getMemo(V);
	return M.Used.count(BB);
}

/// isLiveThroughBlock - Test if the given value is known to be
/// live-through the given block, meaning that the block is
/// dominated by the value's definition, and there exists a block
/// reachable from it that contains a use. 
///
bool Live::isLiveThroughBlock( Value *V,
		BasicBlock *BB) {
	Memo &M = getMemo(V);
	return M.LiveThrough.count(BB);
}

/// isKilledInBlock - Test if the given value is known to be killed in
/// the given block, meaning that the block contains a use of the value,
/// and no blocks reachable from the block contain a use.
///
bool Live::isKilledInBlock( Value *V,  BasicBlock *BB) {
	Memo &M = getMemo(V);
	return M.Killed.count(BB);
}

/// getMemo - Retrieve an existing Memo for the given value if one
/// is available, otherwise compute a new one.
///
Live::Memo &Live::getMemo( Value *V) {
	DenseMap< Value *, Memo>::iterator I = Memos.find(V);
	if (I != Memos.end())
		return I->second;
	return compute(V);
}

/// compute - Compute a new Memo for the given value.
///
Live::Memo &Live::compute( Value *V) {
	Memo &M = Memos[V];

	// Determine the block containing the definition.
	BasicBlock *DefBB;
	// Instructions define values with meaningful live ranges.
	if ( Instruction *I = dyn_cast<Instruction>(V))
		DefBB = I->getParent();
	// Arguments can be analyzed as values defined in the entry block.
	else if ( Argument *A = dyn_cast<Argument>(V))
		DefBB = &A->getParent()->getEntryBlock();
	// Constants and other things aren't meaningful here, so just
	// return having computed an empty Memo so that we don't come
	// here again. The assumption here is that client code won't
	// be asking about such values very often.
	else
		return M;

	// Determine if the value is defined inside a loop. This is used
	// to track whether the value is ever used outside the loop, so
	// it'll be set to null if the value is either not defined in a
	// loop or used outside the loop in which it is defined.
	Loop *L = LI->getLoopFor(DefBB);

	// Track whether the value is used anywhere outside of the block
	// in which it is defined.
	bool LiveOutOfDefBB = false;

	// Examine each use of the value.
	for (Value::use_iterator I = V->use_begin(), E = V->use_end();
			I != E; ++I) {
		User *U = *I;
		BasicBlock *UseBB = cast<Instruction>(U)->getParent();

		// Note the block in which this use occurs.
		M.Used.insert(UseBB);

		// Observe whether the value is used outside of the loop in which
		// it is defined. Switch to an enclosing loop if necessary.
		for (; L; L = L->getParentLoop())
			if (L->contains(UseBB))
				break;

		if (isa<PHINode>(U)) {
			// The value is used by a PHI, so it is live-out of the defining block.
			LiveOutOfDefBB = true;
		} else if (UseBB != DefBB) {
			// A use outside the defining block has been found.
			LiveOutOfDefBB = true;

			// We add to LiveThrough blocks all the blocks that are located
			// between the definition of the value and its use.
			std::stack< BasicBlock *> S;
			S.push(UseBB);
			while (!S.empty()) {
				BasicBlock * BB = S.top();
				S.pop();
				if (!M.LiveThrough.count(BB)) {
					M.LiveThrough.insert(BB);
					if (BB != DefBB) {
						for (pred_iterator p = pred_begin(BB), e = pred_end(BB); 
								p != e; 
								++p){
							BasicBlock * Pred = *p;
							S.push(Pred);
						}
					}
				}
			}
		}
	}

	// If the value is defined inside a loop and is not live outside
	// the the successors of exiting blocks that are outside the loop are Killed
	// blocks
	if (L) {
		SmallVector<BasicBlock *, 4> ExitingBlocks;
		L->getExitingBlocks(ExitingBlocks);
		for (unsigned i = 0, e = ExitingBlocks.size(); i != e; ++i) {
			BasicBlock *ExitingBlock = ExitingBlocks[i];
			for (succ_iterator SI = succ_begin(ExitingBlock), E = succ_end(ExitingBlock); 
					SI != E; ++SI) {
				BasicBlock *Succ = *SI;
				if (!L->contains(Succ))
					M.Killed.insert(Succ);
			}
		}
	}

	// If the value was never used outside the the block in which it was
	// defined, it's killed in that block.
	if (!LiveOutOfDefBB)
		M.Killed.insert(DefBB);

	return M;
}

