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

#include "AISimple.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMT.h"
#include "Debug.h"
#include "Analyzer.h"
#include "PathTree.h"

using namespace llvm;

void AISimple::computeFunc(Function * F, SMT * LSMT, Live * LV, LoopInfo * LI) {
	BasicBlock * b;
	Node * n;
	Node * current;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;
	n = Nodes[b];

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
	n->X_s[passID]->set_top(env);
	n->X_d[passID]->set_top(env);
	A.push(n);

	is_computed.clear();
	// Simple Abstract Interpretation algorithm
	while (!A.empty()) {
		current = A.top();
		A.pop();
		computeNode(current);
	}

	// finally, narrow
	A.push(n);
	is_computed.clear();
	while (!A.empty()) {
		current = A.top();
		A.pop();
		narrowNode(current);
	}

	// then we move X_d abstract values to X_s abstract values
	copy_Xd_to_Xs(F);
}

std::set<BasicBlock*> AISimple::getPredecessors(BasicBlock * b) const {
	std::set<BasicBlock*> preds;
	pred_iterator p = pred_begin(b), E = pred_end(b);
	while (p != E) {
		preds.insert(*p);
		p++;
	}
	return preds;
}

void AISimple::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Abstract * Xtemp;
	Node * Succ;
	std::vector<Abstract*> Join;
	std::list<BasicBlock*> path;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	is_computed[n] = true;
	if (n->X_s[passID]->is_bottom()) {
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

		if (LI->isLoopHeader(Succ->bb)) {
			Xtemp->widening(Succ->X_s[passID]);
		} else {
			Xtemp->join_array_dpUcm(Xtemp->main->env,aman->NewAbstract(Succ->X_s[passID]));
		}

		if ( !Xtemp->is_leq(Succ->X_s[passID])) {
			delete Succ->X_s[passID];
			Succ->X_s[passID] = Xtemp;
			Xtemp = NULL;
			A.push(Succ);
			is_computed[Succ] = false;
		} else {
			delete Xtemp;
		}
		DEBUG(
			*Out << "RESULT FOR BASICBLOCK " << Succ->bb << ":\n";
			Succ->X_s[passID]->print();
		);
	}
}

void AISimple::narrowNode(Node * n) {
	Abstract * Xtemp;
	Node * Succ;
	std::list<BasicBlock*> path;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	is_computed[n] = true;

	for (succ_iterator s = succ_begin(n->bb), E = succ_end(n->bb); s != E; ++s) {
		path.clear();
		path.push_back(n->bb);
		path.push_back(*s);
		Succ = Nodes[*s];

		// computing the image of the abstract value by the path's tranformation
		Xtemp = aman->NewAbstract(n->X_s[passID]);
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
		Xtemp = NULL;
		A.push(Succ);
	}
}
