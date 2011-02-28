#include "Live.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
using namespace llvm;

char Live::ID = 0;
static RegisterPass<Live>
X("live-values", "Value Liveness Analysis", false, true);

Live::Live() : FunctionPass(ID) {}

void Live::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<LoopInfo>();
  AU.setPreservesAll();
}

bool Live::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTree>();
  LI = &getAnalysis<LoopInfo>();

  // This pass' values are computed lazily, so there's nothing to do here.

  return false;
}

void Live::releaseMemory() {
  Memos.clear();
}

/// isUsedInBlock - Test if the given value is used in the given block.
///
bool Live::isUsedInBlock(const Value *V, const BasicBlock *BB) {
  Memo &M = getMemo(V);
  return M.Used.count(BB);
}

/// isLiveThroughBlock - Test if the given value is known to be
/// live-through the given block, meaning that the block is properly
/// dominated by the value's definition, and there exists a block
/// reachable from it that contains a use. This uses a conservative
/// approximation that errs on the side of returning false.
///
bool Live::isLiveThroughBlock(const Value *V,
                                    const BasicBlock *BB) {
  Memo &M = getMemo(V);
  return M.LiveThrough.count(BB);
}

/// isKilledInBlock - Test if the given value is known to be killed in
/// the given block, meaning that the block contains a use of the value,
/// and no blocks reachable from the block contain a use. This uses a
/// conservative approximation that errs on the side of returning false.
///
bool Live::isKilledInBlock(const Value *V, const BasicBlock *BB) {
  Memo &M = getMemo(V);
  return M.Killed.count(BB);
}

/// getMemo - Retrieve an existing Memo for the given value if one
/// is available, otherwise compute a new one.
///
Live::Memo &Live::getMemo(const Value *V) {
  DenseMap<const Value *, Memo>::iterator I = Memos.find(V);
  if (I != Memos.end())
    return I->second;
  return compute(V);
}

/// getImmediateDominator - A handy utility for the specific DominatorTree
/// query that we need here.
///
static const BasicBlock *getImmediateDominator(const BasicBlock *BB,
                                               const DominatorTree *DT) {
  DomTreeNode *Node = DT->getNode(const_cast<BasicBlock *>(BB))->getIDom();
  return Node ? Node->getBlock() : 0;
}

/// compute - Compute a new Memo for the given value.
///
Live::Memo &Live::compute(const Value *V) {
  Memo &M = Memos[V];

  // Determine the block containing the definition.
  const BasicBlock *DefBB;
  // Instructions define values with meaningful live ranges.
  if (const Instruction *I = dyn_cast<Instruction>(V))
    DefBB = I->getParent();
  // Arguments can be analyzed as values defined in the entry block.
  else if (const Argument *A = dyn_cast<Argument>(V))
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
  const Loop *L = LI->getLoopFor(DefBB);

  // Track whether the value is used anywhere outside of the block
  // in which it is defined.
  bool LiveOutOfDefBB = false;

  // Examine each use of the value.
  for (Value::const_use_iterator I = V->use_begin(), E = V->use_end();
       I != E; ++I) {
    const User *U = *I;
    const BasicBlock *UseBB = cast<Instruction>(U)->getParent();

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

      // Climb the immediate dominator tree from the use to the definition
      // and mark all intermediate blocks as live-through. Don't do this if
      // the user is a PHI because such users may not be dominated by the
      // definition.
      for (const BasicBlock *BB = getImmediateDominator(UseBB, DT);
           BB != DefBB; BB = getImmediateDominator(BB, DT))
        if (!M.LiveThrough.insert(BB))
          break;
    }
  }

  // If the value is defined inside a loop and is not live outside
  // the loop, then each exit block of the loop in which the value
  // is used is a kill block.
  if (L) {
    SmallVector<BasicBlock *, 4> ExitingBlocks;
    L->getExitingBlocks(ExitingBlocks);
    for (unsigned i = 0, e = ExitingBlocks.size(); i != e; ++i) {
      const BasicBlock *ExitingBlock = ExitingBlocks[i];
      //if (M.Used.count(ExitingBlock))
        M.Killed.insert(ExitingBlock);
    }
  }

  // If the value was never used outside the the block in which it was
  // defined, it's killed in that block.
  if (!LiveOutOfDefBB)
    M.Killed.insert(DefBB);

  return M;
}

