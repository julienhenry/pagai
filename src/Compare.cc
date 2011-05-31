
#include "Compare.h"
#include "Expr.h"
#include "AI.h"
#include "AIGopan2.h"
#include "Node.h"
#include "Debug.h"
#include "Analyzer.h"

using namespace llvm;

char Compare::ID = 0;
static RegisterPass<Compare>
X("compare", "Abstract values comparison pass", false, true);

const char * Compare::getPassName() const {
	return "Compare";
}

Compare::Compare() : ModulePass(ID) {}

void Compare::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<SMT>();
	AU.addRequired<AI>();
	AU.addRequired<AIGopan2>();
	AU.setPreservesAll();
}

int Compare::compareAbstract(Abstract * A, AbstractGopan * B) {
	ap_environment_t * lcenv = common_environment(
			A->main->env,
			B->main->env);
	A->change_environment(lcenv);
	B->change_environment(lcenv);

	if (A->is_eq(B)) {
		return 0;
	} else if (A->is_leq(B)) {
		return 1;
	} else if (B->is_leq(A)) {
		return -1;
	} else {
		return 0;
	}
}

bool Compare::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = &(getAnalysis<SMT>());
	
	int equal = 0;
	int LW = 0;
	int PF = 0;

	Out->changeColor(raw_ostream::BLUE,true);
	*Out << "\n\n\n"
			<< "------------------------------------------\n"
			<< "-         COMPARING RESULTS              -\n"
			<< "------------------------------------------\n";
	Out->resetColor();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		if (ignoreFunction.count(F) > 0) continue;
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (LSMT->getPr(*b->getParent())->count(b)) {
				switch (compareAbstract(n->Y,n->Xgopan)) {
					case 0:
						equal++;
						break;
					case 1:
						PF++;
						break;
					case -1:
						LW++;
						break;
					default:
						break;
				}
			}
			delete Nodes[b];
		}
	}
	*Out << "\n";
	*Out << PF << " : Path focusing is better\n";
	*Out << LW << " : Lookahead Widening is better\n";
	*Out << equal << " : Same result\n";
	return true;
}

