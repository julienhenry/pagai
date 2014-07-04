#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"


using namespace llvm;

class TagInline : public ModulePass {

	private:	
		static std::vector<const char *> ToAnalyze;

	public:
		static char ID;
		TagInline() : ModulePass(ID) {}

		bool runOnModule(Module &M);

		static ArrayRef<const char *> GetFunctionsToAnalyze();
	
		const char * getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

};

