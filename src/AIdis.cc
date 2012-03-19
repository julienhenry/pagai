#include <vector>
#include <list>

#include "AIdis.h"
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

static RegisterPass<AIdis> X("AIdisPass", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIdis, 0> > Y0("AIdisPass_wrapped0", "Abstract Interpretation Pass", false, true);
static RegisterPass<ModulePassWrapper<AIdis, 1> > Y1("AIdisPass_wrapped1", "Abstract Interpretation Pass", false, true);

char AIdis::ID = 0;

const char * AIdis::getPassName() const {
	return "AIdis";
}

void AIdis::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Pr>();
	AU.addRequired<Live>();
}

bool AIdis::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b = NULL;
	Node * n = NULL;
	int N_Pr = 0;
	LSMT = SMTpass::getInstance();

	*Out << "Starting analysis: DISJUNCTIVE\n";

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


		// we create the new pathtree and Sigma
		std::set<BasicBlock*>* Pr = Pr::getPr(*F); 
		for (std::set<BasicBlock*>::iterator it = Pr->begin(), et = Pr->end();
			it != et;
			it++) {
			pathtree[*it] = new PathTree(*it);
			S[*it] = new Sigma(*it,Max_Disj);
		}

		computeFunction(F);
		*Total_time[passID][F] = sys::TimeValue::now()-*Total_time[passID][F];
		
		printResult(F);

		// we delete the previous pathtree
		for (std::map<BasicBlock*,PathTree*>::iterator it = pathtree.begin(), et = pathtree.end();
			it != et;
			it++) {
			delete (*it).second;
		}
		pathtree.clear();

		// we delete the previous Sigma
		for (std::map<BasicBlock*,Sigma*>::iterator it = S.begin(), et = S.end();
			it != et;
			it++) {
			delete (*it).second;
		}
		S.clear();

	}
	return 0;
}



void AIdis::computeFunction(Function * F) {
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
				return;
			}
		}

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
	}
}

std::set<BasicBlock*> AIdis::getPredecessors(BasicBlock * b) const {
	return Pr::getPrPredecessors(b);
}

std::set<BasicBlock*> AIdis::getSuccessors(BasicBlock * b) const {
	return Pr::getPrSuccessors(b);
}

int AIdis::sigma(
		std::list<BasicBlock*> path, 
		int start,
		Abstract * Xtemp,
		bool source) {
	return S[path.front()]->getSigma(path,start,Xtemp,this,source);
}

void AIdis::computeNewPaths(Node * n) {
	Node * Succ;
	Abstract * Xtemp = NULL;
	std::vector<Abstract*> Join;

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
		DEBUG(
			Out->changeColor(raw_ostream::RED,true);
			*Out << "COMPUTENEWPATHS------ NEW SMT SOLVE -------------------------\n";
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
		int index = 0;
		res = LSMT->SMTsolve(smtexpr,&path,index);
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
		AbstractDisj * Xdisj = dynamic_cast<AbstractDisj*>(n->X_s[passID]);
		Xtemp = Xdisj->man_disj->NewAbstract(Xdisj->getDisjunct(index));

		DEBUG(
			*Out << "START\n";
			Xtemp->print();
		);
		computeTransform(Xdisj->man_disj,n,path,*Xtemp);
		DEBUG(
			*Out << "XTEMP\n";
			Xtemp->print();
		);
		AbstractDisj * SuccDisj = dynamic_cast<AbstractDisj*>(Succ->X_s[passID]);
		int Sigma = sigma(path,index,Xtemp,false);
		Join.clear();
		Join.push_back(SuccDisj->getDisjunct(Sigma));
		Join.push_back(Xdisj->man_disj->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);
		SuccDisj->setDisjunct(Sigma,Xtemp);
		Xtemp = NULL;

		// there is a new path that has to be explored
		pathtree[n->bb]->insert(path,false);
		DEBUG(
			*Out << "INSERTING INTO P THE PATH\n";
			printPath(path);
			*Out << "RESULT\n";
			Succ->X_s[passID]->print();
		);
		A.push(n);
		A.push(Succ);
		//is_computed[Succ] = false;
		A_prime.push(Succ);
	}
}

void AIdis::loopiter(
		Node * n, 
		int index,
		int Sigma,
		Abstract * &Xtemp, 
		std::list<BasicBlock*> * path,
		bool &only_join, 
		PathTree * const U,
		PathTree * const V
		) {
	Node * Succ = n;
	AbstractDisj * SuccDis = dynamic_cast<AbstractDisj*>(Succ->X_s[passID]);

	std::vector<Abstract*> Join;
	if (U->exist(*path)) {
		if (V->exist(*path)) {
			only_join = false;
		} else {
			// backup the previous abstract value
			Abstract * Xpred = SuccDis->man_disj->NewAbstract(SuccDis->getDisjunct(index));

			Join.clear();
			Join.push_back(SuccDis->man_disj->NewAbstract(SuccDis->getDisjunct(Sigma)));
			Join.push_back(SuccDis->man_disj->NewAbstract(Xtemp));
			Xtemp->join_array(Xtemp->main->env,Join);

			DEBUG(
				*Out << "BEFORE MINIWIDENING\n";	
				*Out << "Succ->X:\n";
				SuccDis->print();
				*Out << "Xtemp:\n";
				Xtemp->print();
			);

			DEBUG(
				*Out << "THRESHOLD:\n";
				fflush(stdout);
				ap_lincons1_array_fprint(stdout,&threshold);
				fflush(stdout);
			);
			if (use_threshold)
				Xtemp->widening_threshold(SuccDis->getDisjunct(Sigma),&threshold);
			else
				Xtemp->widening(SuccDis->getDisjunct(Sigma));
			DEBUG(
				*Out << "MINIWIDENING!\n";	
			);
			delete SuccDis->getDisjunct(Sigma);
			SuccDis->setDisjunct(Sigma,Xtemp);
			DEBUG(
				*Out << "AFTER MINIWIDENING\n";	
				Xtemp->print();
			);

			Xtemp = SuccDis->man_disj->NewAbstract(SuccDis->getDisjunct(Sigma));
			computeTransform(SuccDis->man_disj,n,*path,*Xtemp);
			DEBUG(
				*Out << "POLYHEDRON AT THE STARTING NODE (AFTER MINIWIDENING)\n";
				SuccDis->print();
				*Out << "POLYHEDRON AFTER PATH TRANSFORMATION (AFTER MINIWIDENING)\n";
				Xtemp->print();
			);
			
			delete SuccDis->getDisjunct(index);
			SuccDis->setDisjunct(index,Xpred);
			only_join = true;
			V->insert(*path);
		}
	} else {
		only_join = true;
		U->insert(*path);
	}
}

void AIdis::computeNode(Node * n) {
	BasicBlock * const b = n->bb;
	Abstract * Xtemp = NULL;
	Node * Succ = NULL;
	std::vector<Abstract*> Join;
	bool only_join = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}
	std::map<int,PathTree*> U;
	std::map<int,PathTree*> V;

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
			*Out << "COMPUTENODE------- NEW SMT SOLVE -------------------------\n";
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
		int index;
		res = LSMT->SMTsolve(smtexpr,&path,index);
		LSMT->pop_context();
		if (res != 1 || path.size() == 1) {
			if (res == -1) {
				unknown = true;
				// delete all path trees and return
				for (std::map<int,PathTree*>::iterator it = U.begin(), 
						et = U.end(); it != et; it++) 
					delete it->second;
				for (std::map<int,PathTree*>::iterator it = V.begin(), 
						et = V.end(); it != et; it++) 
					delete it->second;
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
		AbstractDisj * Xdisj = dynamic_cast<AbstractDisj*>(n->X_s[passID]);
		Xtemp = Xdisj->man_disj->NewAbstract(Xdisj->getDisjunct(index));
		computeTransform(Xdisj->man_disj,n,path,*Xtemp);
		int Sigma = sigma(path,index,Xtemp,true);
		DEBUG(
			*Out << "POLYHEDRON AT THE STARTING NODE\n";
			n->X_s[passID]->print();
			*Out << "POLYHEDRON AFTER PATH TRANSFORMATION\n";
			Xtemp->print();
		);
		AbstractDisj * SuccDisj = dynamic_cast<AbstractDisj*>(Succ->X_s[passID]);
		SuccDisj->change_environment(Xtemp->main->env,Sigma);

		if (!U.count(index))
			U[index] = new PathTree(n->bb);
		if (!V.count(index))
			V[index] = new PathTree(n->bb);

		// if we have a self loop, we apply loopiter
		if (Succ == n) {
			loopiter(n,index,Sigma,Xtemp,&path,only_join,U[index],V[index]);
		} 
		Join.clear();
		Join.push_back(Xdisj->man_disj->NewAbstract(SuccDisj->getDisjunct(Sigma)));
		Join.push_back(Xdisj->man_disj->NewAbstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);

		if (Pr::inPw(Succ->bb) && ((Succ != n) || !only_join)) {
			if (use_threshold)
				Xtemp->widening_threshold(SuccDisj->getDisjunct(Sigma),&threshold);
			else
				Xtemp->widening(SuccDisj->getDisjunct(Sigma));
			DEBUG(*Out << "WIDENING! \n";);
		} else {
			DEBUG(*Out << "PATH NEVER SEEN BEFORE !!\n";);
		}
		DEBUG(
			*Out << "BEFORE:\n";
			Succ->X_s[passID]->print();
		);
		delete SuccDisj->getDisjunct(Sigma);
		SuccDisj->setDisjunct(Sigma,Xtemp);
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

	//delete U;
	for (std::map<int,PathTree*>::iterator it = U.begin(), et = U.end(); it != et; it++) {
		delete it->second;
	}
	for (std::map<int,PathTree*>::iterator it = V.begin(), et = V.end(); it != et; it++) {
		delete it->second;
	}
}

void AIdis::narrowNode(Node * n) {

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
		int index = 0;
		res = LSMT->SMTsolve(smtexpr,&path,index);
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
		AbstractDisj * Xdisj = dynamic_cast<AbstractDisj*>(n->X_s[passID]);
		Xtemp = Xdisj->man_disj->NewAbstract(Xdisj->getDisjunct(index));
		computeTransform(Xdisj->man_disj,n,path,*Xtemp);
		
		int Sigma = sigma(path,index,Xtemp,false);

		DEBUG(
			*Out << "POLYHEDRON TO JOIN\n";
			Xtemp->print();
		);

		AbstractDisj * SuccDisj = dynamic_cast<AbstractDisj*>(Succ->X_d[passID]);

		if (SuccDisj->getDisjunct(Sigma)->is_bottom()) {
			delete SuccDisj->getDisjunct(Sigma);
			SuccDisj->setDisjunct(Sigma, Xtemp);
		} else {
			std::vector<Abstract*> Join;
			Join.push_back(SuccDisj->man_disj->NewAbstract(SuccDisj->getDisjunct(Sigma)));
			Join.push_back(Xtemp);
			SuccDisj->getDisjunct(Sigma)->join_array(Xtemp->main->env,Join);
		}
		Xtemp = NULL;
		A.push(Succ);
		is_computed[Succ] = false;
	}
}
