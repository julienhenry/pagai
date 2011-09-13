#ifndef _AIGOPAN_H
#define _AIGOPAN_H

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

/// @brief Gopan&Reps Implementation.
///
/// This class is almost identical to AIClassic, the only difference
/// being the abstract domain (which uses a main value to decide which
/// paths are to be explored, and a pilot value to actually compute
/// the invariants).
///
/// There is a lot of code duplication, as a workaround to an LLVM bug
/// (or misunderstanding): if the classes are actually the same, then
/// LLVM considers them as duplicate and runs only one of them.
/// (question asked here:
/// http://thread.gmane.org/gmane.comp.compilers.llvm.devel/42684
/// without answer)
class AIGopan : public ModulePass, public AISimple {

	public:
		static char ID;	

	public:

		AIGopan ():
			ModulePass(ID)
			{
				aman = new AbstractManGopan();
				//aman = new AbstractManClassic();
				passID = LOOKAHEAD_WIDENING;
				Passes[LOOKAHEAD_WIDENING] = passID;	
			}

		~AIGopan () {
			}

		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		bool runOnModule(Module &M);

		void computeFunction(Function * F);
};

#endif
