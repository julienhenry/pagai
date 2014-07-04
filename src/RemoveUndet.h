#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"


using namespace llvm;

class RemoveUndet : public ModulePass {
	
	private:
		std::map<const Type*,Constant*> undet_functions;
		
		Constant* getNondetFn (Module * M, Type *type);

	public:
		static char ID;
		RemoveUndet() : ModulePass(ID) {}

		bool runOnModule(Module &M);

};
