#include <assert.h>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>


#include "taginline.h"
#include "Analyzer.h"

using namespace llvm;

bool TagInline::runOnModule(Module &M) {

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function * F = mIt;
		// if the function is only a declaration, skip
		if (F->begin() == F->end()) continue;
		F->addAttribute(llvm::AttributeSet::FunctionIndex, llvm::Attribute::AlwaysInline);
		if (!definedMain() && F->use_empty()) {
			std::string name = F->getName().str();
			// if SVComp, we focus on the main function only
			//if (!SVComp() || name.compare("main") == 0) {
				ToAnalyze.push_back(F->getName().data());
			//}
		}
		if (definedMain() && isMain(F)) {
			ToAnalyze.push_back(F->getName().data());
		}
	}
	return 0;
}
		
ArrayRef<const char *> TagInline::GetFunctionsToAnalyze() {
	return ArrayRef<const char *>(ToAnalyze);
}

std::vector<const char *> TagInline::ToAnalyze;

const char * TagInline::getPassName() const {
	return "TagInline";
}

void TagInline::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
}

char TagInline::ID = 0;
static RegisterPass<TagInline> X("taginline", "Tag functions for being inlined", false, false);
