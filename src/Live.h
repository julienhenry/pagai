#ifndef LIVE_H
#define LIVE_H

#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"

using namespace llvm;

/// Live - Analysis that provides liveness information for
/// LLVM IR Values.
///
class Live : public FunctionPass {
	LoopInfo *LI;

	/// Memo - A bunch of state to be associated with a value.
	///
	struct Memo {
		/// Used - The set of blocks which contain a use of the value.
		///
		SmallPtrSet< BasicBlock *, 4> Used;
		SmallPtrSet< BasicBlock *, 4> UsedPHI;

		/// LiveThrough - A conservative approximation of the set of blocks in
		/// which the value is live-through, meaning blocks dominated
		/// by the definition, and from which blocks containing uses of the
		/// value are reachable.
		///
		SmallPtrSet< BasicBlock *, 4> LiveThrough;

		/// Killed - the set of blocks in which
		/// the value is not live-out.
		///
		SmallPtrSet< BasicBlock *, 4> Killed;
	};

	/// Memos - Remembers the Memo for each Value. This is populated on
	/// demand.
	///
	DenseMap< Value *, Memo> Memos;

	/// getMemo - Retrieve an existing Memo for the given value if one
	/// is available, otherwise compute a new one.
	///
	Memo &getMemo( Value *V);

	/// compute - Compute a new Memo for the given value.
	///
	Memo &compute( Value *V);

	public:
	static char ID;
	Live();

	const char * getPassName() const;
	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnFunction(Function &F);
	virtual void releaseMemory();

	/// isUsedInBlock - Test if the given value is used in the given block.
	///
	bool isUsedInBlock( Value *V,  BasicBlock *BB);
	bool isUsedInPHIBlock( Value *V,  BasicBlock *BB);

	/// isLiveThroughBlock - Test if the given value is known to be
	/// live-through the given block, meaning that the block is properly
	/// dominated by the value's definition, and there exists a block
	/// reachable from it that contains a use. 	
	///
	bool isLiveThroughBlock( Value *V,  BasicBlock *BB);

	/// isKilledInBlock - Test if the given value is known to be killed in
	/// the given block, meaning that the block contains a use of the value,
	/// and no blocks reachable from the block contain a use.
	///
	bool isKilledInBlock( Value *V,  BasicBlock *BB);
};


#endif

