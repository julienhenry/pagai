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
#include "pk.h"

#include "AIGopan.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMT.h"
#include "Debug.h"
#include "Analyzer.h"
#include "PathTree.h"

using namespace llvm;

static RegisterPass<AIGopan> X("AIGopanPass", "Abstract Interpretation Pass", false, true);

char AIGopan::ID = 0;

const char * AIGopan::getPassName() const {
	return "AIGopan";
}

void AIGopan::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LoopInfo>();
	AU.addRequired<Live>();
	AU.addRequired<SMT>();
}

bool AIGopan::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = &(getAnalysis<SMT>());
	*Out << "Starting analysis: LW\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
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
			//if (LSMT->getPr(*b->getParent())->count(b)) {
				Out->changeColor(raw_ostream::MAGENTA,true);
				*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
				Out->resetColor();
				n->X[passID]->print(true);
			//}
			//delete Nodes[b];
		}
	}

	*Out << "Number of iterations: " << n_iterations << "\n";
	*Out << "Number of paths computed: " << n_paths << "\n";
	return 0;
}


void AIGopan::computeFunction(Function * F) {
	BasicBlock * b;
	Node * n;
	Node * current;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;
	n = Nodes[b];


	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));
	LI = &(getAnalysis<LoopInfo>(*F));

	LSMT->getPr(*F);
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
	n->Y[passID] = aman->NewAbstract(man,env);
	n->Y[passID]->set_top(env);
	A.push(n);

	is_computed.clear();
	// Simple Abstract Interpretation algorithm
	while (!A.empty()) {
		current = A.top();
		A.pop();
		computeNode(current);
	}
}

std::set<BasicBlock*> AIGopan::getPredecessors(BasicBlock * b) const {
	std::set<BasicBlock*> preds;
	pred_iterator p = pred_begin(b), E = pred_end(b);
	while (p != E) {
		preds.insert(*p);
		p++;
	}
	return preds;
}

void AIGopan::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Abstract * Xtemp;
	Node * Succ;
	std::vector<Abstract*> Join;
	std::list<BasicBlock*> path;
	bool update;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	is_computed[n] = true;
	if (n->X[passID]->is_bottom()) {
		return;
	}

	DEBUG (
		Out->changeColor(raw_ostream::GREEN,true);
		*Out << "#######################################################\n";
		*Out << "Computing node: " << b << "\n";
		Out->resetColor();
		*Out << *b << "\n";
	);


	for (succ_iterator s = succ_begin(b), E = succ_end(b); s != E; ++s) {
		path.clear();
		path.push_back(b);
		path.push_back(*s);
		Succ = Nodes[*s];
		update = false;

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X[passID]);
		computeTransform(aman,n,path,*Xtemp);

		DEBUG(
			*Out << "POLYHEDRA AT THE STARTING NODE\n";
			n->X[passID]->print();
			*Out << "POLYHEDRA AFTER PATH TRANSFORMATION\n";
			Xtemp->print();
		);

		Succ->X[passID]->change_environment(Xtemp->main->env);

		if (LI->isLoopHeader(Succ->bb)) {
			Xtemp->widening(Succ->X[passID]);
		} else {
			Xtemp->join_array_dpUcm(Xtemp->main->env,Succ->X[passID]);
		}

		if ( !Xtemp->is_leq(Succ->X[passID])) {
			delete Succ->X[passID];
			Succ->X[passID] = aman->NewAbstract(Xtemp);
			update = true;
		}
		delete Xtemp;
		
		if (update) {
			A.push(Succ);
			is_computed[Succ] = false;
		}
		DEBUG(
			*Out << "RESULT FOR BASICBLOCK " << Succ->bb << ":\n";
			Succ->X[passID]->print();
		);
	}
}

void AIGopan::narrowNode(Node * n) {
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
		// creating the SMT formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true,passID);
		std::list<BasicBlock*> path;
		DEBUG(
			LSMT->man->SMT_print(smtexpr);
		);
		// if the result is unsat, then the computation of this node is finished
		if (!LSMT->SMTsolve(smtexpr,&path) || path.size() == 1) {
			LSMT->pop_context();
			return;
		}
		DEBUG(
			printPath(path);
		);
		
		Succ = Nodes[path.back()];

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X[passID]);
		computeTransform(aman,n,path,*Xtemp);

		DEBUG(
			*Out << "POLYHEDRA TO JOIN\n";
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
