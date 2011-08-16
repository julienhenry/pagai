#ifndef _AIOPT_H
#define _AIOPT_H

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

#include "Analyzer.h"
#include "apron.h"
#include "Node.h"
#include "Live.h"
#include "SMT.h"
#include "PathTree.h"
#include "AIpass.h"

using namespace llvm;

class AIopt : public AIPass {

	private:
		/// paths - remembers all the paths that have already been
		/// visited
		PathTree* pathtree;

		std::set<Node*> A_prime;
		
		bool unknown;
	public:

		AIopt ()  
			{
				//aman = new AbstractManGopan();
				aman = new AbstractManClassic();
				pathtree = new PathTree();
			}

		~AIopt () {
				delete pathtree;
			}

		const char *getPassName() const;

		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);
};

#endif
