#ifndef _AICLASSIC_H
#define _AICLASSIC_H

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
#include "AISimple.h"

using namespace llvm;

/// Pass implementing the basic abstract interpretation algorithm
class AIClassic : public ModulePass, public AISimple {

	public:
		static char ID;	

	public:

		AIClassic ():
			ModulePass(ID)
			{
				aman = new AbstractManClassic();
				passID = SIMPLE;
				Passes[SIMPLE] = passID;	
			}

		~AIClassic () {
			}

		/// @{
		/// @name LLVM pass manager stuff
		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		bool runOnModule(Module &M);
		/// @}

		/// Simple wrapper around AISimple::computeFunc()
		void computeFunction(Function * F);
};

#endif