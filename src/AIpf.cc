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
	*Out << "Starting analysis: PF\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		Total_time[passID][F] = Now();

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
		computeFunction(F);

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (Pr::getPr(*b->getParent())->count(b) && ignoreFunction.count(F) == 0) {
				Out->changeColor(raw_ostream::MAGENTA,true);
				*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
				Out->resetColor();
				n->X_s[passID]->print(true);
				N_Pr++;
			}
			//delete Nodes[b];
		}
		Total_time[passID][F] = sub(Now(),Total_time[passID][F]);
	}

	//*Out << "Number of iterations: " << n_iterations << "\n";
	//*Out << "Number of paths computed: " << n_paths << "\n";

	//*Out << SMT_time.tv_sec << " " << SMT_time.tv_usec  << " SMT_TIME " << "\n";
	//*Out << N_Pr << " PR_SIZE\n";
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
	LI = &(getAnalysis<LoopInfo>(*F));

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
	A.push(n);

	ascendingIter(n, F);
	DEBUG (
		Out->changeColor(raw_ostream::GREEN,true);
		*Out << "#######################################################\n";
		*Out << "NARROWING ITERATIONS\n";
		*Out << "#######################################################\n";
		Out->resetColor();
	);

	// we set X_d abstract values to bottom for narrowing
	// USELESS : they are already at bottom !
	//for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
	//	b = i;
	//	if (Pr::getPr(*F)->count(i) && Nodes[b] != n) {
	//		Nodes[b]->X_d[passID]->set_bottom(env);
	//	}
	//}
	narrowingIter(n);
	// then we move X_d abstract values to X_s abstract values
	while (copy_Xd_to_Xs(F))
		narrowingIter(n);
}

std::set<BasicBlock*> AIpf::getPredecessors(BasicBlock * b) const {
	return Pr::getPrPredecessors(b);
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
	PathTree * const pathtree = new PathTree(n->bb);

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
			delete pathtree;
			return;
		}
	
		DEBUG(
			printPath(path);
		);
	
		Succ = Nodes[path.back()];

		n_iterations++;

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

		if (!pathtree->exist(path)) {
			n_paths++;
			only_join = true;
		} else {
			only_join = false;
		}

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			loopiter(n,Xtemp,&path,only_join,pathtree);
		} 
		
		Join.clear();
		Join.push_back(aman->NewAbstract(Succ->X_s[passID]));
		Join.push_back(aman->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);

		if (LI->isLoopHeader(Succ->bb) && ((Succ != n) || !only_join)) {
				Xtemp->widening(Succ->X_s[passID]);
				//Xtemp->widening_threshold(Succ->X_s[passID],&threshold);
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
			Succ->X_s[passID]->print();
		);
		delete Succ->X_s[passID];
		Succ->X_s[passID] = Xtemp;

		DEBUG(
			*Out << "RESULT:\n";
			Succ->X_s[passID]->print();
		);

		A.push(Succ);
		is_computed[Succ] = false;
	}
	delete pathtree;
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
			*Out << "--------------- NEW SMT SOLVE -------------------------\n";
			Out->resetColor();
		);
		LSMT->push_context();
		// creating the SMTpass formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID);
		std::list<BasicBlock*> path;
		DEBUG_SMT(
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
