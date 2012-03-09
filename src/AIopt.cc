#include <vector>
#include <sstream>
#include <list>

#include "AIopt.h"
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

static RegisterPass<AIopt> X("AIOptPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt, 0> > Y0("AIOptPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt, 1> > Y1("AIOptPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIopt::ID = 0;

const char * AIopt::getPassName() const {
	return "AIopt";
}

void AIopt::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Pr>();
	AU.addRequired<Live>();
}

bool AIopt::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b = NULL;
	Node * n = NULL;
	int N_Pr = 0;
	//LSMT = &(getAnalysis<SMTpass>());
	LSMT = SMTpass::getInstance();
	LSMT->reset_SMTcontext();

	*Out << "Starting analysis: PF+LW\n";

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
			pathtree[*it] = new PathTree(*it);
		}

		computeFunction(F);
		*Total_time[passID][F] = sys::TimeValue::now()-*Total_time[passID][F];
		
		printResult(F);

		// we delete the pathtree
		for (std::map<BasicBlock*,PathTree*>::iterator it = pathtree.begin(), et = pathtree.end();
			it != et;
			it++) {
			delete (*it).second;
		}
		pathtree.clear();
	}
	return 0;
}



void AIopt::computeFunction(Function * F) {
	BasicBlock * b;
	Node * const n = Nodes[F->begin()];
	Node * current;
	unknown = false;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;


	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));

	DEBUG(
		*Out << "Computing Pr...\n";
	);
	Pr::getPr(*F);
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
	
	//A' <- initial state
	A_prime.push(n);

	// Abstract Interpretation algorithm
	while (!A_prime.empty()) {
		// P' <- emptyset
		for (std::map<BasicBlock*,PathTree*>::iterator it = pathtree.begin(), et = pathtree.end(); it != et; it++) {
			(*it).second->clear(true);
		}
		
		// compute the new paths starting in a point in A'
		is_computed.clear();
		while (!A_prime.empty()) {
			Node * current = A_prime.top();
			A_prime.pop();
			computeNewPaths(current); // this method adds elements in A and A'
			if (unknown) {
				ignoreFunction.insert(F);
				while (!A_prime.empty()) A_prime.pop();
				return;
			}
		}

		// P <- P U P'
		for (std::map<BasicBlock*,PathTree*>::iterator it = pathtree.begin(), et = pathtree.end(); it != et; it++) {
			if (!(*it).second->isZero(true)) {
				// we add the new feasible paths to the graph
				(*it).second->mergeBDD();
			}
		}

		W = new PathTree(n->bb);
		is_computed.clear();
		ascendingIter(n, F, true);

		// we set X_d abstract values to bottom for narrowing
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			if (Pr::getPr(*F)->count(i) && Nodes[b] != n) {
				Nodes[b]->X_d[passID]->set_bottom(env);
			}
		}

		narrowingIter(n);

		// then we move X_d abstract values to X_s abstract values
		int step = 0;
		while (copy_Xd_to_Xs(F) && step <= 1) {
			narrowingIter(n);
			step++;
		}
		delete W;

	}
}

std::set<BasicBlock*> AIopt::getPredecessors(BasicBlock * b) const {
	return Pr::getPrPredecessors(b);
}

std::set<BasicBlock*> AIopt::getSuccessors(BasicBlock * b) const {
	return Pr::getPrSuccessors(b);
}

void AIopt::computeNewPaths(Node * n) {
	Node * Succ;
	Abstract * Xtemp = NULL;
	std::vector<Abstract*> Join;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	// first, we set X_d abstract values to X_s
	std::set<BasicBlock*> successors = Pr::getPrSuccessors(n->bb);
	for (std::set<BasicBlock*>::iterator it = successors.begin(),
			et = successors.end();
			it != et;
			it++) {
		Succ = Nodes[*it];
		delete Succ->X_d[passID];
		Succ->X_d[passID] = aman->NewAbstract(Succ->X_s[passID]);
	}

	while (true) {
		is_computed[n] = true;
		DEBUG(
			Out->changeColor(raw_ostream::RED,true);
			*Out << "COMPUTENEWPATHS-------------- SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		// creating the SMTpass formula we want to check
		LSMT->push_context();
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,false,passID,
				pathtree[n->bb]->generateSMTformula(LSMT,true));
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			LSMT->man->SMT_print(smtexpr);
		);
		int res;
		res = LSMT->SMTsolve(smtexpr,&path);
		LSMT->pop_context();

		// if the result is unsat, then the computation of this node is finished
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
				return;
			}
			break;
		}

		Succ = Nodes[path.back()];
		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		computeTransform(aman,n,path,*Xtemp);
		Succ->X_s[passID]->change_environment(Xtemp->main->env);

		Join.clear();
		Join.push_back(Succ->X_s[passID]);
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);
		Succ->X_s[passID] = Xtemp;
		Xtemp = NULL;

		// there is a new path that has to be explored
		pathtree[n->bb]->insert(path,false);
		DEBUG(
			*Out << "THE FOLLOWING PATH IS INSERTED INTO P'\n";	
			printPath(path);
		);
		A.push(n);
		A.push(Succ);
		is_computed[Succ] = false;
		A_prime.push(Succ);
	}
}

void AIopt::computeNode(Node * n) {
	BasicBlock * const b = n->bb;
	Abstract * Xtemp = NULL;
	Node * Succ = NULL;
	std::vector<Abstract*> Join;
	bool only_join = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}
	
	PathTree * const U = new PathTree(n->bb);
	PathTree * const V = new PathTree(n->bb);
	
	DEBUG (
		Out->changeColor(raw_ostream::GREEN,true);
		*Out << "#######################################################\n";
		*Out << "Computing node: " << b << "\n";
		Out->resetColor();
		*Out << *b << "\n";
	);

	while (true) {
		is_computed[n] = true;
		DEBUG(
			Out->changeColor(raw_ostream::RED,true);
			*Out << "COMPUTENODE-------------- NEW SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(b,false,passID,pathtree[b]->generateSMTformula(LSMT));
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		res = LSMT->SMTsolve(smtexpr,&path);
		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
				delete U;
				delete V;
				return;
			}
			break;
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

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			loopiter(n,Xtemp,&path,only_join,U,V);
		} 
		Join.clear();
		Join.push_back(aman->NewAbstract(Succ->X_s[passID]));
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);

		if (Pr::inPw(Succ->bb) && ((Succ != n) || !only_join)) {
				if (W->exist(path)) {
					if (use_threshold)
						Xtemp->widening_threshold(Succ->X_s[passID],&threshold);
					else
						Xtemp->widening(Succ->X_s[passID]);
					DEBUG(*Out << "WIDENING! \n";);
					delete W;
					W = new PathTree(n->bb);
				} else {
					W->insert(path);
				}
		} else {
			DEBUG(*Out << "NO WIDENING\n";);
		}
		DEBUG(
			*Out << "BEFORE:\n";
			Succ->X_s[passID]->print();
		);
		delete Succ->X_s[passID];
		Succ->X_s[passID] = Xtemp;
		Xtemp = NULL;
		DEBUG(
			*Out << "RESULT:\n";
			Succ->X_s[passID]->print();
		);
		A.push(Succ);
		is_computed[Succ] = false;
		// we have to search for new paths starting at Succ, 
		// since the associated abstract value has changed
		A_prime.push(Succ);
	}
	delete U;
	delete V;
}

void AIopt::narrowNode(Node * n) {
	Abstract * Xtemp = NULL;
	Node * Succ;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	while (true) {
		is_computed[n] = true;

		DEBUG(
			Out->changeColor(raw_ostream::RED,true);
			*Out << "NARROWING----------- NEW SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID,pathtree[n->bb]->generateSMTformula(LSMT));
		std::list<BasicBlock*> path;
		DEBUG_SMT(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		res = LSMT->SMTsolve(smtexpr,&path);
		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) unknown = true;
			return;
		}
		DEBUG(
			printPath(path);
		);
		
		Succ = Nodes[path.back()];

		desc_iterations[passID][n->bb->getParent()]++;

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
		computeTransform(aman,n,path,*Xtemp);

		DEBUG(
			*Out << "POLYHEDRON TO JOIN\n";
			Xtemp->print();
			*Out << "POLYHEDRON TO JOIN WITH\n";
			Succ->X_d[passID]->print();
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
		DEBUG(
			*Out << "RESULT\n";
			Succ->X_d[passID]->print();
		);
		Xtemp = NULL;
		A.push(Succ);
		is_computed[Succ] = false;
	}
}
