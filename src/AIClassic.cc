#include "AIClassic.h"

using namespace llvm;

static RegisterPass<AIClassic> X("AIClassicPass", "Abstract Interpretation Pass", false, true);

char AIClassic::ID = 0;

const char * AIClassic::getPassName() const {
	return "AIClassic";
}
