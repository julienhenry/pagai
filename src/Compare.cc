
#include "Compare.h"
#include "Expr.h"
#include "AI.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "Node.h"
#include "Debug.h"

using namespace llvm;

char Compare::ID = 0;
static RegisterPass<Compare>
X("compare", "Abstract values comparison pass", false, true);

const char * Compare::getPassName() const {
	return "Compare";
}

Compare::Compare() : ModulePass(ID) {}

void Compare::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<AI>();
	AU.addRequired<AIopt>();
	AU.addRequired<SMT>();
	AU.addRequired<AIGopan>();
	AU.setPreservesAll();
}

int Compare::compareAbstract(Abstract * A, Abstract * B) {
	ap_environment_t * cenv = intersect_environment(
			A->main->env,
			B->main->env);
	
	A->change_environment(cenv);
	B->change_environment(cenv);

	if (A->is_eq(B)) {
		return 0;
	} else if (A->is_leq(B)) {
		return 1;
	} else if (B->is_leq(A)) {
		return -1;
	} else {
		return -2;
	}
}

void Compare::compareTechniques(Node * n, Techniques t1, Techniques t2) {

	int r = compareAbstract(n->X[t1],n->X[t2]);
	switch (r) {
		case 0:
			results[t1][t2].eq++;
			results[t2][t1].eq++;
			break;
		case 1:
			results[t1][t2].gt++;
			results[t2][t1].lt++;
			break;
		case -1:
			results[t1][t2].lt++;
			results[t1][t2].gt++;
			break;
		case -2:
			results[t1][t2].un++;
			results[t2][t1].un++;
			break;
		default:
			break;
	}
}

void Compare::printResults(Techniques t1, Techniques t2) {

	Out->changeColor(raw_ostream::MAGENTA,true);
	*Out << TechniquesToString(t1) << " - " << TechniquesToString(t2) << "\n";
	Out->resetColor();
	*Out << "EQ " << results[t1][t2].eq << "\n";
	*Out << "LT " << results[t1][t2].lt << "\n";
	*Out << "GT " << results[t1][t2].gt << "\n";
	*Out << "UN " << results[t1][t2].un << "\n";
}

bool Compare::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = &(getAnalysis<SMT>());

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
				compareTechniques(n,LW_WITH_PF,PATH_FOCUSING);
				compareTechniques(n,LW_WITH_PF,LOOKAHEAD_WIDENING);
			}
			delete Nodes[b];
		}
	}
	printResults(LW_WITH_PF,PATH_FOCUSING);
	printResults(LW_WITH_PF,LOOKAHEAD_WIDENING);
	return true;
}

