#include "AIGopan.h"

using namespace llvm;

static RegisterPass<AIGopan> X("AIGopanPass", "Abstract Interpretation Pass", false, true);

char AIGopan::ID = 0;

const char * AIGopan::getPassName() const {
	return "AIGopan";
}
