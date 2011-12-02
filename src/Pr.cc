#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CFG.h"

#include "Pr.h"
#include "Analyzer.h"

using namespace llvm;

char Pr::ID = 0;
static RegisterPass<Pr>
X("Pr set", "Pr set computation pass", false, true);

const char * Pr::getPassName() const {
	return "Pr";
}
		
std::map<Function*,std::set<BasicBlock*>*> Pr::Pr_set;
std::map<BasicBlock*,std::set<BasicBlock*> > Pr::Pr_succ;
std::map<BasicBlock*,std::set<BasicBlock*> > Pr::Pr_pred;

Pr::Pr() : ModulePass(ID) {
}

Pr::~Pr() {
}

void Pr::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfo>();
	AU.setPreservesAll();
}

std::set<BasicBlock*>* Pr::getPr(Function &F) {
	return Pr_set[&F];
}

bool Pr::runOnModule(Module &M) {
	Function * F;

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		if (F->isDeclaration()) continue;
		computePr(*F);
	}
	return 0;
}

// computePr - computes the set Pr of BasicBlocks
// for the moment - Pr = Pw + blocks with a ret inst
void Pr::computePr(Function &F) {
	std::set<BasicBlock*> * FPr = new std::set<BasicBlock*>();
	BasicBlock * b;
	LoopInfo * LI = &(getAnalysis<LoopInfo>(F));

	FPr->insert(F.begin());

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		b = i;
		if (LI->isLoopHeader(b)) {
			FPr->insert(b);
		}

		for (BasicBlock::iterator it = b->begin(), et = b->end(); it != et; ++it) {
			if (isa<ReturnInst>(*it)) {
				FPr->insert(b);
			} else if (CallInst * c = dyn_cast<CallInst>((Instruction*)it)) {
				Function * cF = c->getCalledFunction();
				std::string fname = cF->getName();
				std::string assert_fail ("__assert_fail");
				if (fname.compare(assert_fail) == 0)
					*Out << "FOUND ASSERT\n";
				FPr->insert(b);
			}
		}

	}
	Pr_set[&F] = FPr;
}

std::set<BasicBlock*> Pr::getPrPredecessors(BasicBlock * b) {
	return Pr_pred[b];
}

std::set<BasicBlock*> Pr::getPrSuccessors(BasicBlock * b) {
	return Pr_succ[b];
}

