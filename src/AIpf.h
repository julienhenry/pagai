#ifndef _AI_H
#define _AI_H

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
#include "AIpass.h"

using namespace llvm;

/// Abstract Interpratation with Path Focusing algorithm (using SMT-solving)
class AIpf : public ModulePass, public AIPass {

	public:
		static char ID;	

	public:

		AIpf (): ModulePass(ID)
			{
				//aman = new AbstractManGopan();
				aman = new AbstractManClassic();
				passID = PATH_FOCUSING;
				Passes[PATH_FOCUSING] = passID;	
			}

		~AIpf () {}

		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);
};

#endif
