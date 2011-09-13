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

/// @brief Base class implementing the basic abstract interpretation algorithm
///
/// This class contains the parts of the algorimth common to
/// algorithms that do not use SMT solving to chose the order in which
/// nodes are visited (i.e. Classic, and Gopan&Reps).
class AISimple : public ModulePass, public AIPass {

	public:

		AISimple (char & ID) : ModulePass(ID) {}

		~AISimple () {}
		/// Apply the simple abstract interpretation algorithm
		/// (ascending iterations + narrowing) on function F.
		void computeFunc(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;

		/// @{
		/// @name LLVM pass manager stuff
		virtual const char *getPassName() const = 0;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);
		/// @}

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);

		/// Simple wrapper around AISimple::computeFunc()
		void computeFunction(Function * F);
};

#endif
