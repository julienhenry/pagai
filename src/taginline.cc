#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>


#include "taginline.h"

using namespace llvm;

bool TagInline::runOnModule(Module &M) {

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function * F = mIt;
		// if the function is only a declaration, skip
		if (F->begin() == F->end()) continue;
		F->addAttribute(llvm::AttributeSet::FunctionIndex, llvm::Attribute::AlwaysInline);
	}
	return 0;
}

char TagInline::ID = 0;
static RegisterPass<TagInline> X("taginline", "Tag functions for being inlined", false, false);
