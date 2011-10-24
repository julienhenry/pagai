#include <vector>
#include <list>

#include "llvm/Support/CFG.h"

#include "AISimple.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "Pr.h"
#include "Debug.h"
#include "Analyzer.h"

using namespace llvm;

void AISimple::computeFunc(Function * F) {
	BasicBlock * b;
	Node * n;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;
	n = Nodes[b];

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

	ascendingIter(n, F);

	narrowingIter(n);

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

void AISimple::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LoopInfo>();
	AU.addRequired<Live>();
	AU.addRequired<Pr>();
}

bool AISimple::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	*Out << "Starting analysis: " << getPassName() << "\n";

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		Total_time[passID][F] = Now();
		
		// if the function is only a declaration, do nothing
		if (F->empty()) continue;

		Out->changeColor(raw_ostream::BLUE,true);
		*Out << "\n\n\n"
				<< "------------------------------------------\n"
				<< "-         COMPUTING FUNCTION             -\n"
				<< "------------------------------------------\n";
		Out->resetColor();
		LSMT = SMTpass::getInstance();
		LSMT->reset_SMTcontext();
		initFunction(F);
		computeFunction(F);

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (Pr::getPr(*b->getParent())->count(b)) {
				Out->changeColor(raw_ostream::MAGENTA,true);
				*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
				Out->resetColor();
				n->X_s[passID]->print(true);
			}
			//delete Nodes[b];
		}
		Total_time[passID][F] = sub(Now(),Total_time[passID][F]);
	}

	//*Out << "Number of iterations: " << n_iterations << "\n";
	//*Out << "Number of paths computed: " << n_paths << "\n";
	return 0;
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
			DEBUG(
				*Out << "WIDENING\n";
			);
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

void AISimple::computeFunction(Function * F) {
	if (F->empty()) {
		return;
	}

	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));
	LI = &(getAnalysis<LoopInfo>(*F));

	computeFunc(F);
}
