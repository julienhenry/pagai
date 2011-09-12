#ifndef _AISIMPLE_H
#define _AISIMPLE_H

#include <queue>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Analysis/LoopInfo.h"

#include "ap_global1.h"

#include "apron.h"
#include "Node.h"
#include "Live.h"
#include "SMT.h"
#include "PathTree.h"
#include "AIpass.h"

using namespace llvm;

/// Base class implementing the basic abstract interpretation algorithm
class AISimple : public AIPass {

	public:

		AISimple ()	{}

		~AISimple () {}
		/// Apply the simple abstract interpretation algorithm
		/// (ascending iterations + narrowing) on function F.
		void computeFunc(Function * F);

	private:
		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);
};

#endif
