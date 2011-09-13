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
class AIClassic : public AISimple {

	public:
		/// @brief Pass Identifier
		///
		/// It is crucial for LLVM's pass manager that
		/// this ID is different (in address) from a class to another,
		/// hence this cannot be factored in the base class.
		static char ID;	

	public:

		AIClassic ():
			AISimple(ID)
			{
				aman = new AbstractManClassic();
				passID = SIMPLE;
				Passes[SIMPLE] = passID;	
			}

		~AIClassic () {
			}

		const char *getPassName() const;
};

#endif
