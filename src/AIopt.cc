#include <vector>
#include <list>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/Analysis/Passes.h"

#include "ap_global1.h"

#include "AIopt.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMT.h"
#include "Debug.h"
#include "Analyzer.h"
#include "PathTree.h"

#include "AI.h"
#include "AIGopan.h"

using namespace llvm;

static RegisterPass<AIopt> X("AIOptPass", "Abstract Interpretation Pass", false, true);

char AIopt::ID = 0;

const char * AIopt::getPassName() const {
	return "AIopt";
}

void AIopt::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LoopInfo>();
	AU.addRequired<Live>();
	AU.addRequired<SMT>();
}

bool AIopt::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b = NULL;
	Node * n = NULL;
	int N_Pr = 0;
	LSMT = &(getAnalysis<SMT>());

	*Out << "Starting analysis: PF+LW\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		Out->changeColor(raw_ostream::BLUE,true);
		*Out << "\n\n\n"
				<< "------------------------------------------\n"
				<< "-         COMPUTING FUNCTION             -\n"
				<< "------------------------------------------\n";
		Out->resetColor();
		LSMT->reset_SMTcontext();

		// we delete the previous pathtree, since we entered a new function
		if (pathtree != NULL) {
			delete pathtree;
		}
		pathtree = new PathTree();
		
		initFunction(F);
		computeFunction(F);

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (LSMT->getPr(*b->getParent())->count(b) && ignoreFunction.count(F) == 0) {
				Out->changeColor(raw_ostream::MAGENTA,true);
				*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
				Out->resetColor();
				n->X[passID]->print(true);
				N_Pr++;
			}
			//delete Nodes[b];
		}
	}

	*Out << "Number of iterations: " << n_iterations << "\n";
	*Out << "Number of paths computed: " << n_paths << "\n";

	*Out << SMT_time.tv_sec << " " << SMT_time.tv_usec  << " SMT_TIME " << "\n";
	Total_time = sub(Now(),Total_time);
	*Out << Total_time.tv_sec << " " << Total_time.tv_usec << " TOTAL_TIME\n";
	*Out << N_Pr << " PR_SIZE\n";
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
	LI = &(getAnalysis<LoopInfo>(*F));

	DEBUG(
		*Out << "Computing Pr...\n";
	);
	LSMT->getPr(*F);
	*Out << "Computing Rho...";
	LSMT->getRho(*F);
	*Out << "OK\n";
	
	//LSMT->man->SMT_print(LSMT->getRho(*F));

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
	n->X[passID]->set_top(env);
	delete n->Y[passID];
	n->Y[passID] = aman->NewAbstract(man,env);
	n->Y[passID]->set_top(env);
	A.push(n);

	is_computed.clear();
	// Abstract Interpretation algorithm
	
	while (true) {
		A_prime.clear();	
		pathtree->clear(true);
		while (!A.empty()) {
			current = A.top();
			A.pop();
			computeNode(current);
			if (unknown) {
				ignoreFunction.insert(F);
				while (!A.empty()) A.pop();
				return;
			}
		}
	
		// narrowing 
		is_computed.clear();
		A.push(n);
		while (!A.empty()) {
			current = A.top();
			A.pop();
			narrowNode(current);
		}
		// then we move Y abstract values to X abstract values
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			delete Nodes[b]->X[passID];
			Nodes[b]->X[passID] = aman->NewAbstract(Nodes[b]->Y[passID]);
			if (Nodes[b] != n) {
				delete Nodes[b]->Y[passID];
				Nodes[b]->Y[passID] = aman->NewAbstract(man,env);
			}
		}

		if (pathtree->isZero(true)) {
			break;
		}
		// we add the new feasible paths to the graph
		pathtree->mergeBDD();
		// we insert the new elements in A
		for (std::set<Node*>::iterator it = A_prime.begin(), et = A_prime.end(); it != et; it++) {
			current = *it;
			is_computed[current] = false;
			A.push(current);
		}
		A_prime.clear();
	}
}

std::set<BasicBlock*> AIopt::getPredecessors(BasicBlock * b) const {
	return LSMT->getPrPredecessors(b);
}

void AIopt::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Abstract * Xtemp = NULL;
	Node * Succ = NULL;
	std::vector<Abstract*> Join;
	bool only_join = false;


	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	PathTree * const U = new PathTree();
	
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
			*Out << "-------------- NEW SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		LSMT->push_context();
		// creating the SMT formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,false,passID,pathtree->generateSMTformula(LSMT));
		std::list<BasicBlock*> path;
		DEBUG(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
	
		struct timeval beginTime = Now();
		res = LSMT->SMTsolve(smtexpr,&path);
		
		SMT_time = add(SMT_time,sub(Now(),beginTime));

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

		n_iterations++;

		// computing the image of the abstract value by the path's tranformation
		if (Xtemp != NULL) delete Xtemp;
		Xtemp = aman->NewAbstract(n->X[passID]);
		computeTransform(aman,n,path,*Xtemp);
		
		DEBUG(
			*Out << "POLYHEDRON AT THE STARTING NODE\n";
			n->X[passID]->print();
			*Out << "POLYHEDRON AFTER PATH TRANSFORMATION\n";
			Xtemp->print();
		);

		Succ->X[passID]->change_environment(Xtemp->main->env);

		if (!U->exist(path)) {
			n_paths++;
			only_join = true;
		} else {
			only_join = false;
		}

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			if (U->exist(path)) {
			// backup the previous abstract value
			Abstract * Xpred = aman->NewAbstract(Succ->X[passID]);

			Join.clear();
			Join.push_back(aman->NewAbstract(Xpred));
			Join.push_back(aman->NewAbstract(Xtemp));
			Xtemp->join_array(Xtemp->main->env,Join);

			Xtemp->widening(Succ->X[passID]);
			DEBUG(
				*Out << "MINIWIDENING\n";	
			);
			Succ->X[passID] = Xtemp;

			Xtemp = aman->NewAbstract(n->X[passID]);
			computeTransform(aman,n,path,*Xtemp);
			
			delete Succ->X[passID];
			Succ->X[passID] = Xpred;
			only_join = true;
			U->remove(path);
			if (U->exist(path)) {
				*Out << "ERROR STILL EXIST\n";
			}
			} else {
				U->insert(path);
			}
		} 
		
		Join.clear();
		Join.push_back(aman->NewAbstract(Succ->X[passID]));
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);

		if (LI->isLoopHeader(Succ->bb) && ((Succ != n) || !only_join)) {
				Xtemp->widening(Succ->X[passID]);
				DEBUG(
					*Out << "WIDENING! \n";
				);
		} else {
			DEBUG(
				*Out << "PATH NEVER SEEN BEFORE !!\n";
			);
		}
		
		DEBUG(
			*Out << "BEFORE:\n";
			Succ->X[passID]->print();
		);
		Succ->X[passID] = Xtemp;
		Xtemp = NULL;

		DEBUG(
			*Out << "RESULT:\n";
			Succ->X[passID]->print();
		);

		A.push(Succ);
		is_computed[Succ] = false;
	}

	// now, we check if there exist a new feasible path that has never been
	// computed
	
	// creating the SMT formula we want to check
	LSMT->push_context();
	SMT_expr smtexpr = LSMT->createSMTformula(n->bb,false,passID);
	std::list<BasicBlock*> path;
	DEBUG(
		LSMT->man->SMT_print(smtexpr);
	);
	int res;
	struct timeval beginTime = Now();
	res = LSMT->SMTsolve(smtexpr,&path);
	SMT_time = add(SMT_time,sub(Now(),beginTime));
	LSMT->pop_context();

	// if the result is unsat, then the computation of this node is finished
	if (res != 1 || path.size() == 1) {
		if (res == -1) unknown = true;
	} else {
		// there is a new path that has to be explored
		pathtree->insert(path,true);
		A_prime.insert(n);
	}

	delete U;
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
		// creating the SMT formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID,pathtree->generateSMTformula(LSMT));
		std::list<BasicBlock*> path;
		DEBUG(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		int res;
		res = LSMT->SMTsolve(smtexpr,&path);
		if (res != 1 || path.size() == 1) {
			LSMT->pop_context();
			if (res == -1) unknown = true;
			return;
		}
		DEBUG(
			printPath(path);
		);
		
		Succ = Nodes[path.back()];

		// computing the image of the abstract value by the path's tranformation
		if (Xtemp != NULL) delete Xtemp;
		Xtemp = aman->NewAbstract(n->X[passID]);
		computeTransform(aman,n,path,*Xtemp);

		DEBUG(
			*Out << "POLYHEDRON TO JOIN\n";
			Xtemp->print();
		);

		if (Succ->Y[passID]->is_bottom()) {
			delete Succ->Y[passID];
			Succ->Y[passID] = aman->NewAbstract(Xtemp);
		} else {
			std::vector<Abstract*> Join;
			Join.push_back(aman->NewAbstract(Succ->Y[passID]));
			Join.push_back(aman->NewAbstract(Xtemp));
			Succ->Y[passID]->join_array(Xtemp->main->env,Join);
		}
		A.push(Succ);
		is_computed[Succ] = false;
		LSMT->pop_context();
	}
}
