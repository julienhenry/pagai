#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/FormattedStream.h"

#include "AI.h"
#include "Hashtables.h"

using namespace llvm;

char AI::ID = 0;

const char * AI::getPassName() const {
	return "AI";
}


bool AI::runOnModule(Module &M) {

	
	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function* F = &*mIt;
	}
	return 0;
}


