#include "Compare.h"
#include "Expr.h"
#include "AIpf.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "Node.h"
#include "Debug.h"
#include "ModulePassWrapper.h"

using namespace llvm;

char Compare::ID = 0;
static RegisterPass<Compare>
X("compare", "Abstract values comparison pass", false, true);

const char * Compare::getPassName() const {
	return "Compare";
}

Compare::Compare() : ModulePass(ID) {}

void Compare::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<ModulePassWrapper<AIopt, 0> >();
	AU.addRequired<ModulePassWrapper<AIpf, 0> >();
	AU.addRequired<ModulePassWrapper<AIGopan, 0> >();
	AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
	AU.addRequired<ModulePassWrapper<AIdis, 0> >();
	AU.addRequired<SMTpass>();
	AU.setPreservesAll();
}

int Compare::compareAbstract(Abstract * A, Abstract * B) {
	bool f = false;
	bool g = false;

	ap_environment_t * cenv = intersect_environment(
			A->main->env,
			B->main->env);

	A->change_environment(cenv);
	B->change_environment(cenv);

	LSMT->reset_SMTcontext();
	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,A);
	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,B);

	LSMT->push_context();
	// f = A and not B
	std::vector<SMT_expr> cunj;
	cunj.push_back(A_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		f = true;
	}
	LSMT->pop_context();

	// g = B and not A
	cunj.clear();
	cunj.push_back(B_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
	test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		g = true;
	}

	if (!f && !g) {
		return 0;
	} else if (!f && g) {
		return 1;
	} else if (f && !g) {
		DEBUG(
			*Out << "############################\n";
			B->print();
			*Out << "is leq than \n";
			A->print();
			*Out << "############################\n";
		);
		return -1;
	} else {
		return -2;
	}
}

void Compare::compareTechniques(Node * n, Techniques t1, Techniques t2) {

	switch (compareAbstract(n->X_s[t1],n->X_s[t2])) {
		case 0:
			results[t1][t2].eq++;
			results[t2][t1].eq++;
			break;
		case 1:
			results[t1][t2].lt++;
			results[t2][t1].gt++;
			break;
		case -1:
			results[t1][t2].gt++;
			results[t2][t1].lt++;
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
	*Out << "\n";
	*Out << "EQ " << results[t1][t2].eq << "\n";
	*Out << "LT " << results[t1][t2].lt << "\n";
	*Out << "GT " << results[t1][t2].gt << "\n";
	*Out << "UN " << results[t1][t2].un << "\n";
}

void Compare::printAllResults() {
	*Out	<< "\n";
	*Out	<< "MATRIX:\n";
	*Out	<< results[LOOKAHEAD_WIDENING][SIMPLE].eq << " "
			<< results[LOOKAHEAD_WIDENING][SIMPLE].lt << " "
			<< results[LOOKAHEAD_WIDENING][SIMPLE].gt << " "
			<< results[LOOKAHEAD_WIDENING][SIMPLE].un << " "
			<< "\n";
	*Out	<< results[PATH_FOCUSING][SIMPLE].eq << " "
			<< results[PATH_FOCUSING][SIMPLE].lt << " "
			<< results[PATH_FOCUSING][SIMPLE].gt << " "
			<< results[PATH_FOCUSING][SIMPLE].un << " "
			<< "\n";
	*Out	<< results[PATH_FOCUSING][LOOKAHEAD_WIDENING].eq << " "
			<< results[PATH_FOCUSING][LOOKAHEAD_WIDENING].lt << " "
			<< results[PATH_FOCUSING][LOOKAHEAD_WIDENING].gt << " "
			<< results[PATH_FOCUSING][LOOKAHEAD_WIDENING].un << " "
			<< "\n";
	*Out	<< results[LW_WITH_PF][PATH_FOCUSING].eq << " "
			<< results[LW_WITH_PF][PATH_FOCUSING].lt << " "
			<< results[LW_WITH_PF][PATH_FOCUSING].gt << " "
			<< results[LW_WITH_PF][PATH_FOCUSING].un << " "
			<< "\n";
	*Out	<< results[LW_WITH_PF][LOOKAHEAD_WIDENING].eq << " "
			<< results[LW_WITH_PF][LOOKAHEAD_WIDENING].lt << " "
			<< results[LW_WITH_PF][LOOKAHEAD_WIDENING].gt << " "
			<< results[LW_WITH_PF][LOOKAHEAD_WIDENING].un << " "
			<< "\n";
	*Out	<< results[LW_WITH_PF][SIMPLE].eq << " "
			<< results[LW_WITH_PF][SIMPLE].lt << " "
			<< results[LW_WITH_PF][SIMPLE].gt << " "
			<< results[LW_WITH_PF][SIMPLE].un << " "
			<< "\n";
}

bool Compare::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = &(getAnalysis<SMTpass>());

	Out->changeColor(raw_ostream::BLUE,true);
	*Out << "\n\n\n"
			<< "------------------------------------------\n"
			<< "-         COMPARING RESULTS              -\n"
			<< "------------------------------------------\n";
	Out->resetColor();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;

		if (ignoreFunction.count(F) > 0) continue;
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (LSMT->getPr(*b->getParent())->count(b)) {
				compareTechniques(n,LOOKAHEAD_WIDENING,SIMPLE);
				compareTechniques(n,PATH_FOCUSING,SIMPLE);
				compareTechniques(n,PATH_FOCUSING,LOOKAHEAD_WIDENING);
				compareTechniques(n,LW_WITH_PF,PATH_FOCUSING);
				compareTechniques(n,LW_WITH_PF,LOOKAHEAD_WIDENING);
				compareTechniques(n,LW_WITH_PF,SIMPLE);
				compareTechniques(n,LW_WITH_PF_DISJ,LW_WITH_PF);
			}
		}
	}
	printResults(LOOKAHEAD_WIDENING,SIMPLE);
	printResults(PATH_FOCUSING,SIMPLE);
	printResults(PATH_FOCUSING,LOOKAHEAD_WIDENING);
	printResults(LW_WITH_PF,PATH_FOCUSING);
	printResults(LW_WITH_PF,LOOKAHEAD_WIDENING);
	printResults(LW_WITH_PF,SIMPLE);
	printResults(LW_WITH_PF_DISJ,LW_WITH_PF);

	printAllResults();
	return true;
}

