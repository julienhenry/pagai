/**
 * \file AIpass.cc
 * \brief Implementation of the AIpass pass
 * \author Julien Henry
 */
#include <vector>
#include <list>
#include <fstream>

#include "llvm/IR/Instructions.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/system_error.h"

#include "AIpass.h"
#include "Pr.h"
#include "AISimple.h"
#include "AIGuided.h"
#include "Expr.h"
#include "Live.h"
#include "Node.h"
#include "Debug.h"
#include "recoverName.h"

using namespace llvm;

AIPass * CurrentAIpass = NULL;

void AIPass::ascendingIter(Node * n, Function * F, bool dont_reset) {
	A.push(n);
	if (!dont_reset) {
		is_computed.clear();
	}
	while (!A.empty()) {
		Node * current = A.top();
		A.pop();
		computeNode(current);
		if (unknown) {
			ignoreFunction[passID].insert(F);
			while (!A.empty()) A.pop();
			return;
		}
	}
}

#define NARROWING_LIMIT 10
void AIPass::narrowingIter(Node * n) {
	A.push(n);
	is_computed.clear();
	std::map<Node*, int> narrowing_limit;
	while (!A.empty()) {
		Node * current = A.top();
		//if (narrowing_limit.count(current) == 0)
		//	narrowing_limit.insert(std::pair<Node*,int>(current,0));
		narrowing_limit[current]++;
		A.pop();
		//if (narrowing_limit[current] < NARROWING_LIMIT);
		narrowNode(current);
		if (unknown) {
			ignoreFunction[passID].insert(n->bb->getParent());
			while (!A.empty()) A.pop();
			return;
		}
	}
}

void AIPass::initFunction(Function * F) {
	Node * n;
	CurrentAIpass = this;
	if (preferedOutput() != LLVM_OUTPUT && recoverName::hasMetadata(F)) {
		if (recoverName::is_readable(F)) {
			recoverName::process(F);
			set_useSourceName(true);
		} else {
			set_useSourceName(false);
		}
	} else {
		set_useSourceName(false);
	}
	// we create the Node objects associated to each basicblock
	Pr * FPr = Pr::getInstance(F);
	Environment empty_env;
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		//resetting parameters
		n = Nodes[i];
		n->intVar.clear();
		n->realVar.clear();
		n->setEnv(&empty_env);
		// creating an X_s and an X_d abstract value for this node
		if (LSMT == NULL
				|| !is_SMT_technique()
				|| FPr->inPr(i)) {
			n->X_s[passID] = aman->NewAbstract(man,n->getEnv());
			n->X_d[passID] = aman->NewAbstract(man,n->getEnv());
			n->X_i[passID] = aman->NewAbstract(man,n->getEnv());
			n->X_f[passID] = aman->NewAbstract(man,n->getEnv());
		} else {
			n->X_s[passID] = NULL;
			n->X_d[passID] = NULL;
			n->X_i[passID] = NULL;
			n->X_f[passID] = NULL;
		}
	}

	//if (!quiet_mode()) {
	if (1) {
		Out->changeColor(raw_ostream::BLUE,true);
		*Out  	<< "/* processing Function "<<F->getName()<< " */\n";
		Out->resetColor();
		//if (!useSourceName()) {
		//	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
		//		printBasicBlock(i);
		//}
	}
}

void AIPass::TerminateFunction() {
	// Analysis of the function is finished, we can delete and clear internal
	// data
	if (!threshold_empty) {
		delete threshold;
		threshold_empty = true;
	}
	Expr::clear_exprs();
	PHIvars.name.clear();
	PHIvars.expr.clear();
	PHIvars_prime.name.clear();
	PHIvars_prime.expr.clear();
	focuspath.clear();
}

void format_string(std::string & left) {
	int k;
	for (k = 0; k < left.size(); k++) {
		if (left[k] != '\t')
			left[k] = ' '; 
	}
}

void AIPass::computeResultsPositions(
		Function * F,
		std::map<std::string,std::multimap<std::pair<int,int>,BasicBlock*> > * files 
		) {
	std::string sourcedir = recoverName::getSourceFileDir(F);
	std::string source = recoverName::getSourceFileName(F);
	std::string sourcefile = sourcedir+"/"+source;
	// compute a map associating a (line,column) to a basicblock
	// the map is ordered, so that we can use an iterator for displaying the
	// invariant for the basicblock when needed
	std::multimap<std::pair<int,int>,BasicBlock*> BasicBlock_position; 
	BasicBlock * b;
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		int l = recoverName::getBasicBlockLineNo(b);
		int c = recoverName::getBasicBlockColumnNo(b);
		BasicBlock_position.insert( 
				std::pair<std::pair<int,int>,BasicBlock*>(std::pair<int,int>(l,c),b)
				);
	}

	if (files->count(sourcefile)) {
		(*files)[sourcefile].insert(BasicBlock_position.begin(),BasicBlock_position.end());
	} else {
		files->insert(std::pair<std::string,std::multimap<std::pair<int,int>,BasicBlock*> >(sourcefile,BasicBlock_position));
	}
}

void AIPass::printInvariant(BasicBlock * b, std::string left, llvm::raw_ostream * oss) {
	Pr * FPr = Pr::getInstance(b->getParent());
	if (Nodes[b]->X_s[passID] != NULL && FPr->inPr(b)) {
		// format string in order to remove undesired characters
		format_string(left);
		if (FPr->getAssert()->count(b)) {
			if (Nodes[b]->X_s[passID]->is_bottom()) {
				oss->changeColor(raw_ostream::GREEN,true);
				*oss << "/* assert OK */\n"; 
				oss->resetColor();
			} else {
				oss->changeColor(raw_ostream::RED,true);
				*oss << "/* assert not proved */\n"; 
				oss->resetColor();
			}
			*oss << left;
		} else if (FPr->getUndefinedBehaviour()->count(b)) {
			if (Nodes[b]->X_s[passID]->is_bottom()) {
				oss->changeColor(raw_ostream::GREEN,true);
				*oss << "// safe\n"; 
				oss->resetColor();
				*oss << left;
			} else {
				oss->changeColor(raw_ostream::RED,true);
				*oss << "// unsafe: "; 
				*oss << getUndefinedBehaviourMessage(b) << "\n";
				oss->resetColor();
				*oss << left;
			}
		} else {
			if (Nodes[b]->X_s[passID]->is_bottom()) {
				oss->changeColor(raw_ostream::MAGENTA,true);
				*oss << "/* UNREACHABLE */\n"; 
				oss->resetColor();
			} else if (Nodes[b]->X_s[passID]->is_top()) {
				oss->changeColor(raw_ostream::MAGENTA,true);
				*oss << "/* reachable */\n"; 
				oss->resetColor();
			} else {
				oss->changeColor(raw_ostream::MAGENTA,true);
				*oss << "/* invariant:\n"; 
				Nodes[b]->X_s[passID]->display(*oss,&left);
				*oss << left << "*/\n";
				oss->resetColor();
			}
			*oss << left;
		}
	}
}


void AIPass::InstrumentLLVMBitcode(Function * F) {
	BasicBlock * b;
	Node * n;
	Pr * FPr = Pr::getInstance(F);
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		n = Nodes[b];
		if ((!printAllInvariants() && FPr->inPr(b) && !ignored(F)) ||
		(printAllInvariants() && n->X_s.count(passID) && n->X_s[passID] != NULL && !ignored(F))) {
			Instruction * Inst = b->getFirstNonPHI();
			std::vector<Value*> arr;
			n->X_s[passID]->to_MDNode(Inst,&arr);
			LLVMContext& C = Inst->getContext();
			MDNode* N = MDNode::get(C,arr);
			Inst->setMetadata("pagai.invariant", N);
		}
	}
}

void AIPass::printBasicBlock(BasicBlock* b) {
	Node * n = Nodes[b];
	*Out << "\n(SCC=" << n->sccId << ")" << *b;
}

void AIPass::printPath(std::list<BasicBlock*> path) {
	std::list<BasicBlock*>::iterator i = path.begin(), e = path.end();
	Out->changeColor(raw_ostream::MAGENTA,true);
	*Out << "PATH: ";
	while (i != e) {
		*Out << *i;
		++i;
		if (i != e) *Out << " --> ";
	}
	*Out << "\n";
	//
	i = path.begin();
	while (i != e) {
		*Out << **i;
		++i;
	}
	*Out << "\n";
	//
	Out->resetColor();
}

void AIPass::assert_invariant(
		params P,
		Function * F
		) {
	// we assert b_i => I_i for each block
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		Node * n = Nodes[i];	
		SMT_expr invariant = LSMT->AbstractToSmt(NULL,n->X_s[P]);
		SMT_var bvar = LSMT->man->SMT_mk_bool_var(LSMT->getNodeName(n->bb,false));
		SMT_expr block = LSMT->man->SMT_mk_not(LSMT->man->SMT_mk_expr_from_bool_var(bvar));
		std::vector<SMT_expr> smt;
		smt.push_back(block);
		smt.push_back(invariant);
		LSMT->SMT_assert(LSMT->man->SMT_mk_or(smt));
	}
}

bool AIPass::copy_Xd_to_Xs(Function * F) {
	BasicBlock * b;
	Pr * FPr = Pr::getInstance(F);
	Environment empty_env;
	bool res = false;

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		if (!is_SMT_technique()
				|| FPr->inPr(i)) {
			if (Nodes[b]->X_s[passID]->has_same_environment(Nodes[b]->X_d[passID])) {
				if (!res && !Nodes[b]->X_s[passID]->is_eq(Nodes[b]->X_d[passID]))
					res = true;
			} else {
				res = true;
			}

			delete Nodes[b]->X_s[passID];
			if (b != F->begin()) {
				Nodes[b]->X_s[passID] = Nodes[b]->X_d[passID];
				Nodes[b]->X_d[passID] = aman->NewAbstract(man,&empty_env);
			} else {
				Nodes[b]->X_s[passID] = aman->NewAbstract(Nodes[b]->X_d[passID]);
			}
		}
	}
	return res;
}

void AIPass::copy_Xs_to_Xf(Function * F) {
	BasicBlock * b;
	Pr * FPr = Pr::getInstance(F);

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		if (!is_SMT_technique()
				|| FPr->inPr(i)) {

			delete Nodes[b]->X_f[passID];
			Nodes[b]->X_f[passID] = aman->NewAbstract(Nodes[b]->X_s[passID]);
		}
	}
}

void AIPass::copy_Xf_to_Xs(Function * F) {
	BasicBlock * b;
	Pr * FPr = Pr::getInstance(F);

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		if (!is_SMT_technique()
				|| FPr->inPr(i)) {

			delete Nodes[b]->X_s[passID];
			Nodes[b]->X_s[passID] = aman->NewAbstract(Nodes[b]->X_f[passID]);
		}
	}
}

void AIPass::loopiter(
		Node * n, 
		Abstract * &Xtemp, 
		std::list<BasicBlock*> * path,
		bool &only_join, 
		PathTree * const U,
		PathTree * const V
		) {
	Node * Succ = n;
	std::vector<Abstract*> Join;

	if (U->exist(*path)) {
		if (V->exist(*path)) {
			only_join = false;
		} else {
			// backup the previous abstract value
			Abstract * Xpred = aman->NewAbstract(Succ->X_s[passID]);

			Join.clear();
			Join.push_back(aman->NewAbstract(Xpred));
			Join.push_back(aman->NewAbstract(Xtemp));
			Environment Xtemp_env(Xtemp);
			Xtemp->join_array(&Xtemp_env,Join);

			DEBUG(
					*Out << "BEFORE MINIWIDENING\n";	
					*Out << "Succ->X:\n";
					Succ->X_s[passID]->print();
					*Out << "Xtemp:\n";
					Xtemp->print();
				 );

			//DEBUG(
			//	*Out << "THRESHOLD:\n";
			//	fflush(stdout);
			//	ap_lincons1_array_fprint(stdout,&threshold);
			//	fflush(stdout);
			//);

			if (use_threshold)
				Xtemp->widening_threshold(Succ->X_s[passID],threshold);
			else
				Xtemp->widening(Succ->X_s[passID]);
			DEBUG(
					*Out << "MINIWIDENING!\n";	
				 );
			delete Succ->X_s[passID];
			Succ->X_s[passID] = Xtemp;
			DEBUG(
					*Out << "AFTER MINIWIDENING\n";	
					Xtemp->print();
				 );

			Xtemp = aman->NewAbstract(n->X_s[passID]);
			computeTransform(aman,n,*path,Xtemp);
			DEBUG(
					*Out << "POLYHEDRON AT THE STARTING NODE (AFTER MINIWIDENING)\n";
					n->X_s[passID]->print();
					*Out << "POLYHEDRON AFTER PATH TRANSFORMATION (AFTER MINIWIDENING)\n";
					Xtemp->print();
				 );

			delete Succ->X_s[passID];
			Succ->X_s[passID] = Xpred;
			only_join = true;
			V->insert(*path);
		}
	} else {
		only_join = true;
		U->insert(*path);
	}
}


void AIPass::computeEnv(Node * n) {
	BasicBlock * b = n->bb;
	Node * pred = NULL;
	std::map<Value*,std::set<ap_var_t> >::iterator i, e;
	std::set<ap_var_t>::iterator it, et;

	std::map<Value*,std::set<ap_var_t> > intVars;
	std::map<Value*,std::set<ap_var_t> > realVars;


	std::set<BasicBlock*> preds = getPredecessors(b);
	std::set<BasicBlock*>::iterator p = preds.begin(), E = preds.end();

	if (p == E) {
		// we are in the first basicblock of the function
		Function * F = b->getParent();
		for (Function::arg_iterator a = F->arg_begin(), e = F->arg_end(); a != e; ++a) {
			Argument * arg = a;
			if (!(arg->use_empty()))
				n->add_var(arg);
		}
		return;
	}
	// for each predecessor, we iterate on their variables, and we insert
	// them if they are associated to a value which is still live in our Block
	// We do this for both int and real variables
	for (; p != E; ++p) {
		BasicBlock *pb = *p;
		pred = Nodes[pb];
		if (pred->X_s[passID]->main != NULL) {
			for (i = pred->intVar.begin(), e = pred->intVar.end(); i != e; ++i) {
				if (LV->isLiveByLinearityInBlock((*i).first,b,true)) {
					intVars[(*i).first].insert((*i).second.begin(),(*i).second.end());
				}
			}

			for (i = pred->realVar.begin(), e = pred->realVar.end(); i != e; ++i) {
				if (LV->isLiveByLinearityInBlock((*i).first,b,true)) {
					realVars[(*i).first].insert((*i).second.begin(),(*i).second.end());
				}
			}
		}
	}
	n->intVar.clear();
	n->realVar.clear();
	n->intVar.insert(intVars.begin(), intVars.end());
	n->realVar.insert(realVars.begin(), realVars.end());
}

void computeThreshold(
		ap_tcons1_array_t* C,
		std::vector<ap_lincons1_t> * cons,
		Abstract * A,
		Environment * env) {

	// TODO
	//A->set_top(env);
	//A->meet_tcons_array(C);
	//ap_lincons1_array_t cs = A->to_lincons_array();
	//if (ap_lincons1_array_size(&cs) > 0) {
	//	ap_lincons1_t c = ap_lincons1_array_get(&cs,0);
	//	ap_environment_t * e = ap_lincons1_envref(&c);
	//	if (ap_environment_is_leq(e,env->getEnv())) {
	//		ap_lincons1_t tmp = ap_lincons1_copy(&c);
	//		cons->push_back(tmp);
	//	}
	//}
	//ap_lincons1_array_clear(&cs);
}

void AIPass::computeTransform (AbstractMan * aman, Node * n, std::list<BasicBlock*> path, Abstract * Xtemp) {

	// setting the focus path, such that the instructions can be correctly
	// handled
	focuspath.clear();
	constraints.clear();
	Expr::clear_exprs();
	PHIvars.name.clear();
	PHIvars.expr.clear();
	PHIvars_prime.name.clear();
	PHIvars_prime.expr.clear();
	focuspath.assign(path.begin(), path.end());
	Node * succ = Nodes[focuspath.back()];
	focusblock = 0;
	computeEnv(succ);

	std::list<BasicBlock*>::iterator B = path.begin(), E = path.end();
	for (; B != E; ++B) {
		// visit instructions
		for (BasicBlock::iterator i = (*B)->begin(), e = (*B)->end();
				i != e; ++i) {
			if (focusblock == 0) {
				if (!isa<PHINode>(*i)) {
					visit(*i);
				}
			} else if (focusblock == focuspath.size()-1) {
				if (isa<PHINode>(*i)) {
					visit(*i);
				}
			} else {
				visit(*i);
			}
		}
		focusblock++;
	}
	Environment env(succ,LV);
	{
		Environment Xtemp_env(Xtemp);
		// tmpenv contains all the environment of the starting invariant, plus the
		// new environment variables that have been added along the path
		Environment tmpenv = Environment::common_environment(&env,&Xtemp_env);
		Xtemp->change_environment(&tmpenv);
	}

	// first, we assign the Phi variables defined during the path to the right expressions
	Xtemp->assign_texpr_array(&PHIvars.name,&PHIvars.expr,NULL);

	// We create an Abstract Value that will represent the set of constraints
	// Abstract * ConstraintsAbstract = aman->NewAbstract(man, env);

	ap_var_t var;
	Value * val;
	std::set<ap_var_t> intdims;
	std::set<ap_var_t> realdims;

	env.get_vars_live_in(succ->bb,LV,&intdims,&realdims);

	Environment env2(&intdims,&realdims);

	//std::vector<ap_lincons1_t> cons;
	
	Constraint_array intersect;

	std::list<std::vector<Constraint*>*>::iterator i, e;
	for (i = constraints.begin(), e = constraints.end(); i!=e; ++i) {
		if ((*i)->size() == 1) {
			DEBUG(
					*Out << "Constraint: ";
					((*i)->front())->print();
					*Out << "\n";
				 );
			//Xtemp->meet_tcons_array((*i)->front());
			intersect.add_constraint((*i)->front());
			//*Out << "constraint : \n";
			//(*i)->front()->print();
			//*Out << "\n";
			//computeThreshold((*i)->front(),&cons,ConstraintsAbstract,env2);

			// delete the single Constraint_array*
			//delete (*i)->front();
		} else {
			DEBUG(
					*Out << "multiple contraints:\n";
				 );
			std::vector<Abstract*> A;
			// A_Constraints is used by ConstraintsAbstract
			std::vector<Abstract*> A_Constraints;
			std::vector<Constraint*>::iterator it, et;
			Abstract * X2;
			for (it = (*i)->begin(), et = (*i)->end(); it != et; ++it) {
				X2 = aman->NewAbstract(Xtemp);
				Constraint_array intersect_all(*it);
				X2->meet_tcons_array(&intersect_all);
				A.push_back(X2);
				//computeThreshold(*it,&cons,ConstraintsAbstract,env2);
			}
			Environment Xtemp_env(Xtemp);
			Xtemp->join_array(&Xtemp_env,A);
			DEBUG(
					*Out << "multiple contraints OK\n";
				 );
		}
		// delete the vector
		delete *i;
	}
	if (intersect.size() > 0) {
		DEBUG(
			*Out << "intersecting with constraints\n";
			intersect.print();
			*Out << "\n";
		 );
		Xtemp->meet_tcons_array(&intersect);
		DEBUG(
			*Out << "intersecting with constraints OK\n";
		 );
	}


	DEBUG(
			for (unsigned i = 0; i < PHIvars_prime.name.size(); i++) {
			ap_var_t name = PHIvars_prime.name[i];
			Expr expr = PHIvars_prime.expr[i];
			*Out << "Assigning " << ap_var_to_string(name) << " = ";
			expr.print();
			*Out << "\n";
			}
		 );
	Xtemp->assign_texpr_array(&PHIvars_prime.name,&PHIvars_prime.expr,NULL);

	succ->setEnv(&env2);
	Xtemp->change_environment(&env2);

	// we transform the abstract value of constraints into an array of lincons
	// for a future widening with threshold
	if (!threshold_empty) {
		delete threshold;
	}
	//threshold = ap_lincons1_array_make(env2.getEnv(),cons.size());
	threshold_empty = false;
	threshold = new Constraint_array();
	//for (unsigned k = 0; k < cons.size(); k++) {
	//	if (!ap_lincons1_extend_environment_with(&cons[k],env2.getEnv())) {
	//		ap_lincons1_array_set(&threshold,k,&cons[k]);
	//	}
	//}
	//delete ConstraintsAbstract;
}

// TODO :
// make it work for path-focused techniques
bool AIPass::computeWideningSeed(Function * F) {
	Abstract * Xtemp;
	Node * n;
	Node * Succ;
	Pr * FPr = Pr::getInstance(F);
	std::list<BasicBlock*> path;
	bool found = false;

	numNarrowingSeedsInFunction.insert(std::pair<Function*,int>(F,0));
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		n = Nodes[i];
		std::set<BasicBlock*> Succs = getSuccessors(n->bb);
		for (std::set<BasicBlock*>::iterator s = Succs.begin(), E = Succs.end(); s != E; ++s) {

			// to be candidate, the transition should go to a widening point
			if (!FPr->inPw(*s)) continue;

			path.clear();
			path.push_back(n->bb);
			path.push_back(*s);
			Succ = Nodes[*s];
			// computing the image of the abstract value by the path's tranformation
			Xtemp = aman->NewAbstract(n->X_s[passID]);
			computeTransform(aman,n,path,Xtemp);

			// we check if the Abstract value is a good seed for Halbwachs's
			// narrowing
			Environment Xtemp_env(Xtemp);
			Abstract * XSucci = aman->NewAbstract(Succ->X_i[passID]);
			XSucci->change_environment(&Xtemp_env);

			if (!Xtemp->is_leq(XSucci)) {

				Abstract * Xseed = aman->NewAbstract(Xtemp);
				std::vector<Abstract*> Join;
				Join.push_back(aman->NewAbstract(Xtemp));
				Join.push_back(aman->NewAbstract(Succ->X_i[passID]));
				Environment Xtemp_env(Xtemp);
				Xseed->join_array(&Xtemp_env,Join);

				Environment CommonEnv(Succ->X_s[passID]);
				CommonEnv = Environment::common_environment(&CommonEnv,&Xtemp_env);
				Xseed->change_environment(&CommonEnv);
				
				Abstract * XSucc = aman->NewAbstract(Succ->X_s[passID]);
				XSucc->change_environment(&CommonEnv);

				if (Xseed->compare(XSucc) == 1) {
					DEBUG(
							*Out << "n\n";
							n->X_s[passID]->print();
							*Out << "Xtemp\n";
							Xtemp->print();
							*Out << "Succ\n";
							Succ->X_s[passID]->print();
							*Out << "SEED FOUND: " << *(n->bb) << "\n";
						 );
					Join.clear();
					Join.push_back(aman->NewAbstract(Xtemp));
					Join.push_back(aman->NewAbstract(Succ->X_d[passID]));
					Succ->X_d[passID]->join_array(&CommonEnv,Join);
					Succ->X_d[passID]->change_environment(&Xtemp_env);
					A.push(Succ);
					found = true;

					numNarrowingSeedsInFunction[F]++;
				}  
				delete XSucc;
				delete Xseed;
			}
			delete XSucci;
			delete Xtemp;
		}
	}
	return found;
}

bool isequal(std::list<BasicBlock*> p, std::list<BasicBlock*> q) {

	if (p.size() != q.size()) return false;
	std::list<BasicBlock*> P,Q;
	P.assign(p.begin(),p.end());
	Q.assign(q.begin(),q.end());

	while (P.size() > 0) {
		if (P.front() != Q.front()) return false;
		P.pop_front();
		Q.pop_front();
	}
	return true;
}

/* 
 * this function takes all apron variables of an environment, and adds them into
 * the Node's variables, with a Value V as a use.
 */
void AIPass::insert_env_vars_into_node_vars(Environment * env, Node * n, Value * V) {
	std::set<ap_var_t> intvars;
	std::set<ap_var_t> realvars;
	env->get_vars(&intvars,&realvars);

	n->intVar[V].insert(intvars.begin(),intvars.end());
	n->realVar[V].insert(realvars.begin(),realvars.end());
}

bool AIPass::computeCondition(Value * val, 
		bool result,
		int cons_index,
		std::vector< std::vector<Constraint*> * > * cons) {

	bool res;
	if (CmpInst * cmp = dyn_cast<CmpInst>(val)) {
		ap_texpr_rtype_t ap_type;
		if (Expr::get_ap_type(cmp->getOperand(0), ap_type)) {
			return false;
		}
		res = computeCmpCondition(cmp,result,cons_index,cons);
	} else if (ConstantInt * c = dyn_cast<ConstantInt>(val)) {
		res = computeConstantCondition(c,result,cons_index,cons);
	} else if (PHINode * phi = dyn_cast<PHINode>(val)) {
		res = computePHINodeCondition(phi,result,cons_index,cons);
	} else if (BinaryOperator * binop = dyn_cast<BinaryOperator>(val)) {
		res = computeBinaryOpCondition(binop,result,cons_index,cons);
	} else {
		// loss of precision...
		return false;
	}
	return res;
}

bool AIPass::computeCmpCondition(	CmpInst * inst, 
		bool result,
		int cons_index,
		std::vector< std::vector<Constraint*> * > * cons) {

	//Node * n = Nodes[inst->getParent()];
	Node * n = Nodes[focuspath.back()];
	ap_constyp_t constyp;
	ap_constyp_t nconstyp;

	Value * op1 = inst->getOperand(0);
	Value * op2 = inst->getOperand(1);

	Expr exp1(op1);
	Expr exp2(op2);
	Expr::common_environment(&exp1,&exp2);

	ap_texpr_rtype_t ap_type;
	Expr::get_ap_type(op1,ap_type);

	Expr expr(AP_TEXPR_SUB,&exp1,&exp2,ap_type,AP_RDIR_RND);
	Expr nexpr(AP_TEXPR_SUB,&exp2,&exp1,ap_type,AP_RDIR_RND);
	Expr swap(&expr);

	switch (inst->getPredicate()) {
		case CmpInst::FCMP_FALSE:
		case CmpInst::FCMP_OEQ: 
		case CmpInst::FCMP_UEQ: 
		case CmpInst::ICMP_EQ:
			constyp = AP_CONS_EQ; // equality constraint
			nconstyp = AP_CONS_DISEQ;
			break;
		case CmpInst::ICMP_UGT: 
		case CmpInst::ICMP_SGT: 
			// in the case of the false constraint, we have to add 1
			// to the nexpr

		case CmpInst::FCMP_OGT:
		case CmpInst::FCMP_UGT:
			constyp = AP_CONS_SUP; // > constraint
			nconstyp = AP_CONS_SUPEQ;
			break;
		case CmpInst::FCMP_OLT: 
		case CmpInst::FCMP_ULT: 
		case CmpInst::ICMP_ULT: 
		case CmpInst::ICMP_SLT: 
			expr = nexpr;
			nexpr = swap;
			constyp = AP_CONS_SUP; // > constraint
			nconstyp = AP_CONS_SUPEQ; 
			break;
		case CmpInst::FCMP_OGE:
		case CmpInst::FCMP_UGE:
		case CmpInst::ICMP_UGE:
		case CmpInst::ICMP_SGE:
			constyp = AP_CONS_SUPEQ; // >= constraint
			nconstyp = AP_CONS_SUP;
			break;
		case CmpInst::FCMP_OLE:
		case CmpInst::FCMP_ULE:
		case CmpInst::ICMP_ULE:
		case CmpInst::ICMP_SLE:
			expr = nexpr;
			nexpr = swap;
			constyp = AP_CONS_SUPEQ; // >= constraint
			nconstyp = AP_CONS_SUP;
			break;
		case CmpInst::FCMP_ONE:
		case CmpInst::FCMP_UNE: 
		case CmpInst::ICMP_NE: 
		case CmpInst::FCMP_TRUE: 
			constyp = AP_CONS_DISEQ; // disequality constraint
			nconstyp = AP_CONS_EQ;
			break;
		case CmpInst::FCMP_ORD: 
		case CmpInst::FCMP_UNO:
		case CmpInst::BAD_ICMP_PREDICATE:
		case CmpInst::BAD_FCMP_PREDICATE:
			*Out << "ERROR : Unknown predicate\n";
			break;
		default:
			// unreachable
			constyp = AP_CONS_DISEQ; // disequality constraint
			nconstyp = AP_CONS_EQ;
			break;
	}
	if (result) {
		// creating the TRUE constraints
		Expr::create_constraints(constyp,&expr,&nexpr,(*cons)[cons_index]);
	} else {
		// creating the FALSE constraints
		Expr::create_constraints(nconstyp,&nexpr,&expr,(*cons)[cons_index]);
	}
	return true;
}

bool AIPass::computeConstantCondition(	ConstantInt * inst, 
		bool result,
		int cons_index,
		std::vector< std::vector<Constraint*> * > * cons) {

	bool is_null = inst->isNullValue();
	if ((is_null && result) || (is_null && result)) {
		// we create a unsat constraint
		// such as one of the successor is unreachable
		Constraint * c;
		Expr one(1.);

		c = new Constraint(
				AP_CONS_EQ,
				&one,
				NULL);

		// condition is always false
		(*cons)[cons_index]->push_back(c);
		return true;
	} else {
		// there is no constraint 
		return false;
	}
}

bool AIPass::computeBinaryOpCondition(BinaryOperator * inst, 
		bool result,
		int cons_index,
		std::vector< std::vector<Constraint*> * > * cons) {

	bool res = false;
	switch(inst->getOpcode()) {
		case Instruction::Or:
			// cond takes two constraints, one per operand
			if (result) {
				res = computeCondition(inst->getOperand(0),result,cons_index,cons);
				res |= computeCondition(inst->getOperand(1),result,cons_index,cons);
			} else {
				res = computeCondition(inst->getOperand(0),result,cons_index,cons);
				// overapproximation: we do not consider the second operand
				if (cons->size() < cons_index+2) {
					cons->push_back(new std::vector<Constraint*>());
				}
				res |= computeCondition(inst->getOperand(1),result,cons_index+1,cons);
			}
			break;
		case Instruction::And:
			// cond takes one constraint
			// which should be the intersection of the two operands
			if (result) {
				res = computeCondition(inst->getOperand(0),result,cons_index,cons);
				if (cons->size() < cons_index+2) {
					cons->push_back(new std::vector<Constraint*>());
				}
				res |= computeCondition(inst->getOperand(1),result,cons_index+1,cons);
			} else {
				// not (A and B) is not A or not B
				res = computeCondition(inst->getOperand(0),result,cons_index,cons);
				res |= computeCondition(inst->getOperand(1),result,cons_index,cons);
			}
			break;
		case Instruction::Xor:
			// if the xor encodes a not, we can be precise,
			// otherwise we just ignore...
			if (BinaryOperator::isNot(inst)) {
				Value * notval = BinaryOperator::getNotArgument(inst);
				res = computeCondition(notval,!result,cons_index,cons);
			}
			break;
		default:
			break;
	}
	return res;
}

bool AIPass::computePHINodeCondition(PHINode * inst, 
		bool result,
		int cons_index,
		std::vector< std::vector<Constraint*> * > * cons) {

	bool res = false;

	// this is a special case : if we are in the first block of the path, 
	// we can't take the previous block of the path.
	// Then, we loose precision and return false.
	if (focusblock == 0) return false;

	// we only consider one single predecessor: the predecessor from the path
	BasicBlock * pred = focuspath[focusblock-1];
	Value * pv;
	for (unsigned i = 0; i < inst->getNumIncomingValues(); i++) {
		if (pred == inst->getIncomingBlock(i)) {
			pv = inst->getIncomingValue(i);
			res = computeCondition(pv,result,cons_index,cons);
		}
	}
	return res;
}

void AIPass::visitReturnInst (ReturnInst &I){
	//*Out << "returnInst\n" << I << "\n";
}

void AIPass::visitBranchInst (BranchInst &I){
	//*Out << "BranchInst\n" << I << "\n";	
	bool test;
	bool res;

	//*Out << "BranchInst\n" << I << "\n";	
	if (I.isUnconditional()) {
		/* no constraints */
		return;
	}
	// branch under condition

	// what is the successor in our path ?
	// i.e: Does the test has to be 'false' or 'true' ?
	BasicBlock * next = focuspath[focusblock+1];
	if (next == I.getSuccessor(0)) {
		// test is evaluated at true
		test = true;
	} else {
		// test is evaluated at false
		test = false;
	}

	std::vector< std::vector<Constraint*> * > * cons = new std::vector< std::vector<Constraint*> * >();
	cons->push_back(new std::vector<Constraint*>());
	res = computeCondition(I.getOperand(0),test,0,cons);

	// we add cons in the set of constraints of the path
	for (unsigned i = 0; i < cons->size(); i++) {
		if ((*cons)[i]->size() > 0) 
			constraints.push_back((*cons)[i]);
		else
			delete (*cons)[i];
	}
	delete cons;
}

/*
 * This code is dead since we use the LowerSwitch pass before doing abstract
 * interpretation.
 */
void AIPass::visitSwitchInst (SwitchInst &I){
	//*Out << "SwitchInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitIndirectBrInst (IndirectBrInst &I){
	//*Out << "IndirectBrInst\n" << I << "\n";	
}

void AIPass::visitInvokeInst (InvokeInst &I){
	//*Out << "InvokeInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MINOR == 0
void AIPass::visitUnwindInst (UnwindInst &I){
	//*Out << "UnwindInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}
#endif

void AIPass::visitUnreachableInst (UnreachableInst &I){
	//*Out << "UnreachableInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitICmpInst (ICmpInst &I){
	//*Out << "ICmpInst\n" << I << "\n";	
	//visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitFCmpInst (FCmpInst &I){
	//*Out << "FCmpInst\n" << I << "\n";	
	//visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitAllocaInst (AllocaInst &I){
	//*Out << "AllocaInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitLoadInst (LoadInst &I){
	//*Out << "LoadInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitStoreInst (StoreInst &I){
	//*Out << "StoreInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitGetElementPtrInst (GetElementPtrInst &I){
	//*Out << "GetElementPtrInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitPHINode (PHINode &I){
	Node * n = Nodes[focuspath.back()];
	//*Out << "PHINode\n" << I << "\n";
	// we only consider one single predecessor: the predecessor from the path
	BasicBlock * pred = focuspath[focusblock-1];

	Value * pv;
	Node * nb;

	ap_texpr_rtype_t ap_type;
	if (Expr::get_ap_type((Value*)&I, ap_type)) return;
	DEBUG(
			*Out << I << "\n";
		 );

	// if the PHINode has actually one single incoming edge, we just say the
	// value is equal to its associated expression
	// There is no need to introduce PHIvars...
	if (I.getNumIncomingValues() == 1) {
		pv = I.getIncomingValue(0);
		Expr expr(pv);
		Expr::set_expr(&I,&expr);
		Environment * env = expr.getEnv();
		insert_env_vars_into_node_vars(env,n,(Value*)&I);
		delete env;
		return;
	}

	for (unsigned i = 0; i < I.getNumIncomingValues(); i++) {
		if (pred == I.getIncomingBlock(i)) {
			pv = I.getIncomingValue(i);
			nb = Nodes[I.getIncomingBlock(i)];
			Expr expr(pv);

			if (focusblock == focuspath.size()-1) {
				if (LV->isLiveByLinearityInBlock(&I,n->bb,true)) {
					n->add_var(&I);
					if (isa<UndefValue>(pv)) continue;
					PHIvars_prime.name.push_back((ap_var_t)&I);
					PHIvars_prime.expr.push_back(new Expr(expr));
					Environment * env = expr.getEnv();
					insert_env_vars_into_node_vars(env,n,(Value*)&I);
					delete env;
					DEBUG(
							*Out << I << " is equal to ";
							expr.print();
							*Out << "\n";
						 );
				}
			} else {
				DEBUG(
						*Out << I << " is equal to ";
						expr.print();
						*Out << "\n";
					 );
				if (LV->isLiveByLinearityInBlock(&I,n->bb,true)) {
					n->add_var(&I);
					if (isa<UndefValue>(pv)) continue;
					PHIvars.name.push_back((ap_var_t)&I);
					PHIvars.expr.push_back(new Expr(expr));
				} else {
					Expr::set_expr(&I,&expr);
				}
				Environment * env = expr.getEnv();
				insert_env_vars_into_node_vars(env,n,(Value*)&I);
				delete env;
			}
		}
	}
}

void AIPass::visitTruncInst (TruncInst &I){
	//*Out << "TruncInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitZExtInst (ZExtInst &I){
	//*Out << "ZExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitSExtInst (SExtInst &I){
	//*Out << "SExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitFPTruncInst (FPTruncInst &I){
	//*Out << "FPTruncInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitFPExtInst (FPExtInst &I){
	//*Out << "FPExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitFPToUIInst (FPToUIInst &I){
	//*Out << "FPToUIInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitFPToSIInst (FPToSIInst &I){
	//*Out << "FPToSIInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitUIToFPInst (UIToFPInst &I){
	//*Out << "UIToFPInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitSIToFPInst (SIToFPInst &I){
	//*Out << "SIToFPInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitPtrToIntInst (PtrToIntInst &I){
	//*Out << "PtrToIntInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitIntToPtrInst (IntToPtrInst &I){
	//*Out << "IntToPtrInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitBitCastInst (BitCastInst &I){
	//*Out << "BitCastInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitSelectInst (SelectInst &I){
	//*Out << "SelectInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

// a call instruction is treated as follow : 
// we consider that the function call doesn't modify the value of the different
// variable, and we the result returned by the function is a new variable of
// type int or float, depending on the return type
void AIPass::visitCallInst(CallInst &I){
	//*Out << "CallInst\n" << I << "\n";	

	//Function * F = I.getCalledFunction();
	//std::string fname = F->getName();
	//*Out << "FOUND FUNCTION " << fname << "\n";

	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitVAArgInst (VAArgInst &I){
	//*Out << "VAArgInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitExtractElementInst (ExtractElementInst &I){
	//*Out << "ExtractElementInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitInsertElementInst (InsertElementInst &I){
	//*Out << "InsertElementInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitShuffleVectorInst (ShuffleVectorInst &I){
	//*Out << "ShuffleVectorInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitExtractValueInst (ExtractValueInst &I){
	//*Out << "ExtractValueInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitInsertValueInst (InsertValueInst &I){
	//*Out << "InsertValueInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitTerminatorInst (TerminatorInst &I){
	//*Out << "TerminatorInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitBinaryOperator (BinaryOperator &I){
	Node * n = Nodes[focuspath.back()];

	ap_texpr_rtype_t ap_type;
	if (Expr::get_ap_type((Value*)&I, ap_type)) return;
	Expr exp((Value*)&I);

	// this value may use some apron variables 
	// we add these variables in the Node's variable structure, such that we
	// remember that instruction I uses these variables
	//
	Environment * env = exp.getEnv();
	insert_env_vars_into_node_vars(env,n,(Value*)&I);
	delete env;
}

void AIPass::visitCmpInst (CmpInst &I){
	//*Out << "CmpInst\n" << I << "\n";	
	//visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitCastInst (CastInst &I){
	//*Out << "CastInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AIPass::visitInstAndAddVarIfNecessary(Instruction &I) {
	Node * n = Nodes[focuspath.back()];
	ap_var_t var = (Value *) &I; 

	ap_texpr_rtype_t ap_type;
	if (Expr::get_ap_type((Value*)&I, ap_type)) return;

	if (!LV->isLiveByLinearityInBlock(&I,I.getParent(),false))
		return;

	Expr exp(&I);
	Expr::set_expr(&I,&exp);
	if (LV->isLiveByLinearityInBlock(&I,n->bb,true))
		n->add_var((Value*)var);
}


void AIPass::ClearPathtreeMap(std::map<BasicBlock*,PathTree*> & pathtree) {
	for (std::map<BasicBlock*,PathTree*>::iterator it = pathtree.begin(), et = pathtree.end();
			it != et;
			it++) {
		delete (*it).second;
	}
	pathtree.clear();
}
