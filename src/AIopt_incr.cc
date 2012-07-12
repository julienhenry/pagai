#include <vector>
#include <sstream>
#include <list>

#include "AIopt_incr.h"
#include "AIClassic.h"
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

static RegisterPass<AIopt_incr> X("AIOpt_incrPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt_incr, 0> > Y0("AIOpt_incrPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIopt_incr, 1> > Y1("AIOpt_incrPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIopt_incr::ID = 0;

const char * AIopt_incr::getPassName() const {
	return "AIopt_incr";
}

void AIopt_incr::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Pr>();
	AU.addRequired<Live>();
	AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
}

bool AIopt_incr::runOnModule(Module &M) {
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

		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;
		if (definedMain() && getMain().compare(F->getName().str()) != 0) continue;

		Out->changeColor(raw_ostream::BLUE,true);
		*Out << "\n\n\n"
				<< "------------------------------------------\n"
				<< "-         COMPUTING FUNCTION             -\n"
				<< "------------------------------------------\n";
		Out->resetColor();
		LSMT->reset_SMTcontext();

		sys::TimeValue * time = new sys::TimeValue(0,0);
		*time = sys::TimeValue::now();
		Total_time[passID][F] = time;
		
		initFunction(F);


		// we create the new pathtree
		std::set<BasicBlock*>* Pr = Pr::getPr(*F);
		for (std::set<BasicBlock*>::iterator it = Pr->begin(), et = Pr->end();
			it != et;
			it++) {
			pathtree[*it] = new PathTree(*it);
			U[*it] = new PathTree(*it);
			V[*it] = new PathTree(*it);
		}

		computeFunction(F);
		*Total_time[passID][F] = sys::TimeValue::now()-*Total_time[passID][F];
		
		printResult(F);

		// deleting the pathtrees
		ClearPathtreeMap(pathtree);
		ClearPathtreeMap(U);
		ClearPathtreeMap(V);
	}
	LSMT->reset_SMTcontext();
	return 0;
}



void AIopt_incr::computeFunction(Function * F) {
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
	
	LSMT->push_context();
	
	*Out << "Computing Rho...";
	LSMT->SMT_assert(LSMT->getRho(*F));

	/////
	// we assert b_i => I_i for each block
	params P;
	P.T = SIMPLE;
	P.D = getApronManager();
	P.N = useNewNarrowing();
	P.TH = useThreshold();
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		Node * n = Nodes[i];	
		SMT_expr invariant = LSMT->AbstractToSmt(NULL,n->X_s[P]);
		SMT_var bvar = LSMT->man->SMT_mk_bool_var(LSMT->getNodeName(b,false));
		SMT_expr block = LSMT->man->SMT_mk_not(LSMT->man->SMT_mk_expr_from_bool_var(bvar));
		std::vector<SMT_expr> smt;
		smt.push_back(block);
		smt.push_back(invariant);
		LSMT->SMT_assert(LSMT->man->SMT_mk_or(smt));
	}

	/////
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
	
	while (!A_prime.empty()) 
			A_prime.pop();
	while (!A.empty()) 
			A.pop();

	//A' <- initial state
	A_prime.push(n);

	// Abstract Interpretation algorithm
	while (!A_prime.empty()) {
		
		// compute the new paths starting in a point in A'
		is_computed.clear();
		while (!A_prime.empty()) {
			Node * current = A_prime.top();
			A_prime.pop();
			computeNewPaths(current); // this method adds elements in A and A'
			if (unknown) {
				ignoreFunction.insert(F);
				while (!A_prime.empty()) A_prime.pop();
				LSMT->pop_context();
				return;
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
	LSMT->pop_context();
}

std::set<BasicBlock*> AIopt_incr::getPredecessors(BasicBlock * b) const {
	return Pr::getPrPredecessors(b);
}

std::set<BasicBlock*> AIopt_incr::getSuccessors(BasicBlock * b) const {
	return Pr::getPrSuccessors(b);
}

void AIopt_incr::computeNewPaths(Node * n) {
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
			*Out
				<< "\n"
				<< "FORMULA"
				<< "(COMPUTENEWPATHS)"
				<< "\n\n";
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

		// intersection with the previous invariant
		params P;
		P.T = SIMPLE;
		P.D = getApronManager();
		P.N = useNewNarrowing();
		P.TH = useThreshold();
		Xtemp->meet(Succ->X_s[P]);
		//

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
		//is_computed[Succ] = false;
		A_prime.push(Succ);
	}
}

void AIopt_incr::computeNode(Node * n) {
	BasicBlock * const b = n->bb;
	Abstract * Xtemp = NULL;
	Node * Succ = NULL;
	std::vector<Abstract*> Join;
	bool only_join = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	U[n->bb]->clear();
	V[n->bb]->clear();
	
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
			loopiter(n,Xtemp,&path,only_join,U[n->bb],V[n->bb]);
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
					W->clear();
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

		// intersection with the previous invariant
		params P;
		P.T = SIMPLE;
		P.D = getApronManager();
		P.N = useNewNarrowing();
		P.TH = useThreshold();
		Xtemp->meet(Succ->X_s[P]);
		//

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
}

void AIopt_incr::narrowNode(Node * n) {
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
			*Out
				<< "\n"
				<< "FORMULA"
				<< "(NARROWNODE)"
				<< "\n\n";
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
