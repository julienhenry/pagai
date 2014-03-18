#include "llvm/Config/config.h"
#include "llvm/Support/CFG.h"
#include "llvm/InstVisitor.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"

#include <map>

using namespace llvm;

class instrOverflow : public FunctionPass,
	public InstVisitor<instrOverflow, bool> {

		private:
			std::map<CallInst*, Instruction*> replaced;

		public:
			static char ID;
			instrOverflow() : FunctionPass(ID) {}

			bool runOnFunction(Function &F);
			bool updateFunction(Function &F);

			void replaceWithUsualOp(
					Instruction * old, 
					unsigned intrinsicID,
					std::vector<Value*> * args,
					CallInst * intrinsicCall
					);

			void replaceWithCmp(
					Instruction * old, 
					unsigned intrinsicID,
					CallInst * intrinsicCall
					);

			bool visitExtractValueInst(ExtractValueInst &inst);
			bool visitBranchInst(BranchInst &inst);

			bool visitInstruction(Instruction &inst) {return false;}
	};

