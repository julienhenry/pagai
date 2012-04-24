#include <vector>
#include <list>

#include "AIpf.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMTpass.h"
#include "Pr.h"
#include "Debug.h"
#include "Analyzer.h"
#include "PathTree.h"
#include "ModulePassWrapper.h"

using namespace llvm;

static RegisterPass<AIpf> X("AIPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIpf, 0> > Y0("AIpfPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIpf, 1> > Y1("AIpfPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIpf::ID = 0;

const char * AIpf::getPassName() const {
	return "AIpf";
}

void AIpf::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Pr>();
	AU.addRequired<Live>();
}

bool AIpf::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	int N_Pr = 0;
	LSMT = SMTpass::getInstance();
	LSMT->reset_SMTcontext();
	*Out << "Starting analysis: PF\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;

		sys::TimeValue * time = new sys::TimeValue(0,0);
		*time = sys::TimeValue::now();
		Total_time[passID][F] = time;

		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;
		
		Out->changeColor(raw_ostream::BLUE,true);
		*Out << "\n\n\n"
				<< "------------------------------------------\n"
				<< "-         COMPUTING FUNCTION             -\n"
				<< "------------------------------------------\n";
		Out->resetColor();
		LSMT->reset_SMTcontext();
		initFunction(F);
		
		// we create the new pathtree
		std::set<BasicBlock*>* Pr = Pr::getPr(*F);
		for (std::set<BasicBlock*>::iterator it = Pr->begin(), et = Pr->end();
			it != et;
			it++) {
			U[*it] = new PathTree(*it);
			V[*it] = new PathTree(*it);
		}

		computeFunction(F);
		*Total_time[passID][F] = sys::TimeValue::now()-*Total_time[passID][F];

		printResult(F);

		// deleting the pathtrees
		ClearPathtreeMap(U);
		ClearPathtreeMap(V);
	}
	LSMT->reset_SMTcontext();
	return false;
}



void AIpf::computeFunction(Function * F) {
	BasicBlock * b;
	Node * n;
	Node * current;
	unknown = false;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;
	n = Nodes[b];


	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));

	*Out << "Computing Rho...";
	LSMT->getRho(*F);
	*Out << "OK\n";

	
	// add all function's arguments into the environment of the first bb
	for (Function::arg_iterator a = F->arg_begin(), e = F->arg_end(); a != e; ++a) {
		Argument * arg = a;
		if (!(arg->use_empty()))
			n->add_var(arg);
		else 
			*Out << "argument " << *a << " never used !\n";
	}
	// first abstract value is top
	ap_environment_t * env = NULL;
	computeEnv(n);
	n->create_env(&env,LV);
	n->X_s[passID]->set_top(env);
	n->X_d[passID]->set_top(env);
	n->X_i[passID]->set_top(env);
	A.push(n);

	ascendingIter(n, F);

	narrowingIter(n);
	// then we move X_d abstract values to X_s abstract values
	int step = 0;
	while (copy_Xd_to_Xs(F) && step <= 1) {
		narrowingIter(n);
		step++;
	}
}

std::set<BasicBlock*> AIpf::getPredecessors(BasicBlock * b) const {
	return Pr::getPrPredecessors(b);
}

std::set<BasicBlock*> AIpf::getSuccessors(BasicBlock * b) const {
	return Pr::getPrSuccessors(b);
}

void AIpf::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Abstract * Xtemp;
	Node * Succ;
	std::vector<Abstract*> Join;
	bool only_join = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}
	
	DEBUG (
		Out->changeColor(raw_ostream::GREEN,true);
		*Out << "#######################################################\n";
		*Out << "Computing node: " << b << "\n";
		Out->resetColor();
		*Out << *b << "\n";
	);
	U[n->bb]->clear();
	V[n->bb]->clear();

	while (true) {
		is_computed[n] = true;
		DEBUG(
			Out->changeColor(raw_ostream::RED,true);
			*Out << "--------------- NEW SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,false,passID);
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			*Out
				<< "\n"
				<< "FORMULA"
				<< "(COMPUTENODE)"
				<< "\n\n";
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
	
		res = LSMT->SMTsolve(smtexpr,&path);
		
		LSMT->pop_context();

		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
			}
			return;
		}
	
		DEBUG(
			printPath(path);
		);
	
		Succ = Nodes[path.back()];

		asc_iterations[passID][n->bb->getParent()]++;

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		computeTransform(aman,n,path,*Xtemp);
		
		DEBUG(
			*Out << "POLYHEDRON AT THE STARTING NODE\n";
			n->X_s[passID]->print();
			*Out << "POLYHEDRON AFTER PATH TRANSFORMATION\n";
			Xtemp->print();
		);

		Succ->X_s[passID]->change_environment(Xtemp->main->env);

		bool succ_bottom = (Succ->X_s[passID]->is_bottom());

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			loopiter(n,Xtemp,&path,only_join,U[n->bb],V[n->bb]);
		} 
		
		Join.clear();
		Join.push_back(aman->NewAbstract(Succ->X_s[passID]));
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);

		if (Pr::inPw(Succ->bb) && ((Succ != n) || !only_join)) {
			if (use_threshold)
				Xtemp->widening_threshold(Succ->X_s[passID],&threshold);
			else
				Xtemp->widening(Succ->X_s[passID]);
			DEBUG(
				*Out << "WIDENING! \n";
			);
		} else {
			DEBUG(
				*Out << "NO WIDENING\n";
			);
		}
		
		DEBUG(
			*Out << "BEFORE:\n";
			Succ->X_s[passID]->print();
		);
		delete Succ->X_s[passID];
		if (succ_bottom) {
			delete Succ->X_i[passID];
			Succ->X_i[passID] = aman->NewAbstract(Xtemp);
		}
		Succ->X_s[passID] = Xtemp;

		DEBUG(
			*Out << "RESULT:\n";
			Succ->X_s[passID]->print();
		);

		A.push(Succ);
		is_computed[Succ] = false;
	}
}

void AIpf::narrowNode(Node * n) {
	Abstract * Xtemp;
	Node * Succ;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	while (true) {
		is_computed[n] = true;

		DEBUG(
			Out->changeColor(raw_ostream::RED,true);
			*Out << "NARROWING------ NEW SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID);
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			*Out
				<< "\n"
				<< "FORMULA"
				<< "(NARROWNODE)"
				<< "\n\n";
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res = LSMT->SMTsolve(smtexpr,&path);
		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
			}
			return;
		}
		DEBUG(
			printPath(path);
		);
		
		desc_iterations[passID][n->bb->getParent()]++;
		
		Succ = Nodes[path.back()];

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		DEBUG(
			*Out << "STARTING POLYHEDRON\n";
			Xtemp->print();
		);
		computeTransform(aman,n,path,*Xtemp);

		DEBUG(
			*Out << "POLYHEDRON TO JOIN\n";
			Xtemp->print();
		);

		if (Succ->X_d[passID]->is_bottom()) {
			delete Succ->X_d[passID];
			Succ->X_d[passID] = Xtemp;
		} else {
			std::vector<Abstract*> Join;
			Join.push_back(aman->NewAbstract(Succ->X_d[passID]));
			Join.push_back(Xtemp);
			Succ->X_d[passID]->join_array(Xtemp->main->env,Join);
		}
		A.push(Succ);
		is_computed[Succ] = false;
	}
}
