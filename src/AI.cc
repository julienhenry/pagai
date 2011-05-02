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

#include "AI.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "Live.h"
#include "SMT.h"
#include "Debug.h"
#include "Analyzer.h"

using namespace llvm;

char AI::ID = 0;

static RegisterPass<AI> X("AI", "Abstract Interpretation Pass", false, true);

const char * AI::getPassName() const {
	return "AI";
}

void AI::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LoopInfo>();
	AU.addRequired<Live>();
	AU.addRequired<SMT>();
}

bool AI::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = &(getAnalysis<SMT>());

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
	}

	Out->changeColor(raw_ostream::BLUE,true);
	*Out << "\n\n\n"
			<< "------------------------------------------\n"
			<< "-         RESULT OF THE ANALYSIS         -\n"
			<< "------------------------------------------\n";
	Out->resetColor();

	std::map<BasicBlock*,Node*>::iterator it;
	for ( it=Nodes.begin() ; it != Nodes.end(); it++ ) {
		b = it->first;
		n = Nodes[b];
		if (LSMT->getPr(*b->getParent())->count(b)) {
			Out->changeColor(raw_ostream::MAGENTA,true);
			*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
			Out->resetColor();
			Out->flush();
			n->Y->print(true);
		}
		delete it->second;
	}
	return 0;
}

void AI::initFunction(Function * F) {
	Node * n;
	// we create the Node objects associated to each basicblock
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			n = new Node(man,i);
			Nodes[i] = n;
	}
	if (F->size() > 0) {
		// we find the Strongly Connected Components
		Node * front = Nodes[&(F->front())];
		front->computeSCC();
	}
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
		printBasicBlock(i);
	Out->flush();
}

void AI::printBasicBlock(BasicBlock* b) {
	Node * n = Nodes[b];
	LoopInfo &LI = getAnalysis<LoopInfo>(*(b->getParent()));
	if (LI.isLoopHeader(b)) {
		*Out << b << ": SCC=" << n->sccId << ": LOOP HEAD" << *b;
	} else {
		*Out << b << ": SCC=" << n->sccId << ":\n" << *b;
	}
	//for (BasicBlock::iterator i = b->begin(), e = b->end(); i != e; ++i) {
	//	Instruction * I = i;
	//	*Out << "\t\t" << I << "\t" << *I << "\n";
	//}

}

void AI::printPath(std::list<BasicBlock*> path) {
	std::list<BasicBlock*>::iterator i = path.begin(), e = path.end();
	Out->changeColor(raw_ostream::MAGENTA,true);
	*Out << "PATH: ";
	while (i != e) {
		*Out << *i;
		++i;
		if (i != e) *Out << " --> ";
	}
	*Out << "\n";
	Out->resetColor();
}

void AI::computeFunction(Function * F) {
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
	LSMT->getRho(*F);
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
	n->create_env(&env);
	n->X->set_top(env);
	n->Y = new Abstract(man,env);
	n->Y->set_top(env);
	A.push(n);

	// Simple Abstract Interpretation algorithm
	while (!A.empty()) {
		current = A.top();
		A.pop();
		computeNode(current);
	}
	
	is_computed.clear();
	A.push(n);

	DEBUG (
		Out->changeColor(raw_ostream::GREEN,true);
		*Out << "#######################################################\n";
		*Out << "NARROWING ITERATIONS\n";
		*Out << "#######################################################\n";
		Out->resetColor();
		Out->flush();
	);

	// narrowing phase
	while (!A.empty()) {
		current = A.top();
		A.pop();
		narrowNode(current);
	}
}

void AI::computeEnv(Node * n) {
	BasicBlock * b = n->bb;
	Node * pred = NULL;
	std::map<ap_var_t,std::set<Value*> >::iterator i, e;
	std::set<Value*>::iterator it, et;

	std::map<ap_var_t,std::set<Value*> > intVars;
	std::map<ap_var_t,std::set<Value*> > realVars;


	std::set<BasicBlock*> preds = LSMT->getPrPredecessors(b);
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
		if (pred->X->main != NULL) {
			for (i = pred->intVar.begin(), e = pred->intVar.end(); i != e; ++i) {
				std::set<Value*> S;
				for (it = (*i).second.begin(), et = (*i).second.end(); it != et; ++it) {
					Value * v = *it;
					if (LV->isLiveThroughBlock(v,b)) 
						S.insert(v);
				}
				if (S.size()>0)
					intVars[(*i).first].insert(S.begin(),S.end());
			}

			for (i = pred->realVar.begin(), e = pred->realVar.end(); i != e; ++i) {
				std::set<Value*> S;
				for (it = (*i).second.begin(), et = (*i).second.end(); it != et; ++it) {
					Value * v = *it;
					if (LV->isLiveThroughBlock(v,b)) 
						S.insert(v);
				}
				if (S.size()>0)
					realVars[(*i).first].insert(S.begin(),S.end());
			}
		}
	}

	//n->intVar.clear();
	//n->realVar.clear();
	n->intVar.insert(intVars.begin(), intVars.end());
	n->realVar.insert(realVars.begin(), realVars.end());
}

void AI::computeTransform (Node * n, std::list<BasicBlock*> path, Abstract &Xtemp) {
	
	// setting the focus path, such that the instructions can be correctly
	// handled
	focuspath.clear();
	constraints.clear();
	PHIvars.name.clear();
	PHIvars.expr.clear();
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

	ap_environment_t * env = NULL;
	succ->create_env(&env);

	//Xtemp.set_top(env);
	Xtemp.change_environment(env);


	std::vector<ap_lincons1_array_t> linconsts;
	size_t linconssize = 0;

	std::list<std::vector<ap_tcons1_array_t*>*>::iterator i, e;
	for (i = constraints.begin(), e = constraints.end(); i!=e; ++i) {
		if ((*i)->size() == 1) {
				tcons1_array_print((*i)->front());
				Xtemp.meet_tcons_array((*i)->front());
				// creating the associated lincons for a future widening with
				// threshold
				ap_abstract1_t AX = ap_abstract1_of_tcons_array(man,Xtemp.main->env,(*i)->front());
				ap_lincons1_array_t lincons = ap_abstract1_to_lincons_array(man,&AX);
				linconssize += ap_lincons1_array_size(&lincons);
				linconsts.push_back(lincons);
				ap_abstract1_clear(man,&AX);
		} else {
			std::vector<Abstract*> A;
			std::vector<ap_tcons1_array_t*>::iterator it, et;
			Abstract * X2;
			for (it = (*i)->begin(), et = (*i)->end(); it != et; ++it) {
				X2 = new Abstract(&Xtemp);
				X2->meet_tcons_array(*it);
				A.push_back(X2);
				// creating the associated lincons for a future widening with
				// threshold
				ap_abstract1_t AX = ap_abstract1_of_tcons_array(man,Xtemp.main->env,(*it));
				ap_lincons1_array_t lincons = ap_abstract1_to_lincons_array(man,&AX);
				linconssize += ap_lincons1_array_size(&lincons);
				linconsts.push_back(lincons);
				ap_abstract1_clear(man,&AX);
			}
			Xtemp.join_array(env,A);
		}
	}

	//
	linconstraints = ap_lincons1_array_make(Xtemp.main->env,linconssize);
	size_t x = 0;
	size_t N = linconsts.size();
	for (size_t k = 0; k < N; k++) {
		for (size_t j=0; j < ap_lincons1_array_size(&linconsts[k]); j++) {
			ap_lincons1_t cons = ap_lincons1_array_get(&linconsts[k],j);
			ap_lincons1_array_set(&linconstraints,x,&cons);	
			x++;
		}
	}
//	ap_abstract1_t A = ap_abstract1_of_lincons_array(man,Xtemp.main->env,&linconstraints);
//	A = ap_abstract1_assign_texpr_array(man,false,&A,&PHIvars.name[0],&PHIvars.expr[0],PHIvars.name.size(),NULL);
//	linconstraints = ap_abstract1_to_lincons_array(man,&A);

	//

	Xtemp.assign_texpr_array(&PHIvars.name[0],&PHIvars.expr[0],PHIvars.name.size(),NULL);
}


void AI::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Abstract * Xtemp;
	Node * Succ;

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


	while (true) {
		is_computed[n] = true;
		Out->changeColor(raw_ostream::RED,true);
		*Out << "--------------- NEW SMT SOLVE -------------------------\n";
		Out->resetColor();
		Out->flush();
		LSMT->push_context();
		// creating the SMT formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,false);
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
			Out->flush();
		);
		
		Succ = Nodes[path.back()];

		// computing the image of the abstract value by the path's tranformation
		Xtemp = new Abstract(n->X);
		computeTransform(n,path,*Xtemp);
		
		DEBUG(
			*Out << "POLYHEDRA AFTER PATH TRANSFORMATION\n";
			Out->flush();
			Xtemp->print();
		);

		std::vector<Abstract*> Join;
		Join.push_back(new Abstract(Succ->X));
		Join.push_back(new Abstract(Xtemp));
		Xtemp->join_array(Xtemp->main->env,Join);

		Succ->X->change_environment(Xtemp->main->env);

		if (LI->isLoopHeader(Succ->bb)) {
			if (Succ->widening == 2) {
				Xtemp->widening(Succ);
				//Xtemp->widening_threshold(Succ, &linconstraints);
				Succ->widening = 0;

				// if we have a self loop, we narrow
				if (Succ == n) {
					// backup the previous abstract value
					Abstract * Xpred = new Abstract(Succ->X);

					Join.clear();
					Join.push_back(new Abstract(Xpred));
					Join.push_back(new Abstract(Xtemp));
					Xtemp->join_array(Xtemp->main->env,Join);
					Succ->X = Xtemp;

					Xtemp = new Abstract(n->X);
					computeTransform(n,path,*Xtemp);

					Join.clear();
					Join.push_back(Xpred);
					Join.push_back(new Abstract(Xtemp));
					Xtemp->join_array(Xtemp->main->env,Join);
					
				}
			} else {
				Succ->widening++;
			}
		} 
		ap_lincons1_array_clear(&linconstraints);
		
		Succ->X = Xtemp;

		DEBUG(
			*Out << "RESULT:\n";
			Out->flush();
			Succ->X->print();
		);

		A.push(Succ);
		is_computed[Succ] = false;
		LSMT->pop_context();
	}
}

void AI::narrowNode(Node * n) {
	Abstract * Xtemp;
	Node * Succ;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}

	while (true) {
		is_computed[n] = true;

		Out->changeColor(raw_ostream::RED,true);
		*Out << "--------------- NEW SMT SOLVE -------------------------\n";
		Out->resetColor();
		Out->flush();
		LSMT->push_context();
		// creating the SMT formula we want to check
		SMT_expr smtexpr = LSMT->createSMTformula(n->bb,true);
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
			Out->flush();
		);
		
		Succ = Nodes[path.back()];

		// computing the image of the abstract value by the path's tranformation
		Xtemp = new Abstract(n->X);
		computeTransform(n,path,*Xtemp);

		DEBUG(
			*Out << "POLYHEDRA TO JOIN\n";
			Out->flush();
			Xtemp->print();
		);

		if (Succ->Y->is_bottom()) {
			delete Succ->Y;
			Succ->Y = new Abstract(Xtemp);
		} else {
			std::vector<Abstract*> Join;
			Join.push_back(new Abstract(Succ->Y));
			Join.push_back(new Abstract(Xtemp));
			Succ->Y->join_array(Xtemp->main->env,Join);
		}
		A.push(Succ);
		LSMT->pop_context();
	}
}

/// insert_env_vars_into_node_vars - this function takes all apron variables of
/// an environment, and adds them into the Node's variables, with a Value V as
/// a use.
void insert_env_vars_into_node_vars(ap_environment_t * env, Node * n, Value * V) {
	ap_var_t var;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		n->intVar[var].insert(V);
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		n->realVar[var].insert(V);
	}
}

void AI::visitReturnInst (ReturnInst &I){
	//*Out << "returnInst\n" << I << "\n";
}

// create_constraints - this function is called by computeCondition
// it creates the constraint from its arguments and insert it into t_cons
void create_constraints(
	ap_constyp_t constyp,
	ap_texpr1_t * expr,
	ap_texpr1_t * nexpr,
	std::vector<ap_tcons1_array_t*> * t_cons) {
	
	ap_tcons1_t cons;
	ap_tcons1_array_t * consarray;
	
	if (constyp == AP_CONS_DISEQ) {
		// we have a disequality constraint. We tranform it into 2 different
		// constraints: < and >, in order to create further 2 abstract domain
		// instead of one.
		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				AP_CONS_SUP,
				expr,
				ap_scalar_alloc_set_double(0));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		t_cons->push_back(consarray);

		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				AP_CONS_SUP,
				ap_texpr1_copy(nexpr),
				ap_scalar_alloc_set_double(0));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		t_cons->push_back(consarray);
	} else {
		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				constyp,
				expr,
				ap_scalar_alloc_set_double(0));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		t_cons->push_back(consarray);
	}
}


void AI::computeCondition(	CmpInst * inst, 
		bool result,
		std::vector<ap_tcons1_array_t *> * cons) {

	//Node * n = Nodes[inst->getParent()];
	Node * n = Nodes[focuspath.back()];
	ap_constyp_t constyp;
	ap_constyp_t nconstyp;
	
	Value * op1 = inst->getOperand(0);
	Value * op2 = inst->getOperand(1);

	ap_texpr1_t * exp1 = get_ap_expr(n,op1);
	ap_texpr1_t * exp2 = get_ap_expr(n,op2);
	common_environment(&exp1,&exp2);

	ap_texpr1_t * expr;
	ap_texpr1_t * nexpr;
	ap_texpr1_t * swap;

	ap_texpr_rtype_t ap_type;
	get_ap_type(op1,ap_type);

	expr = ap_texpr1_binop(AP_TEXPR_SUB,
			ap_texpr1_copy(exp1),
			ap_texpr1_copy(exp2),
			ap_type,
			AP_RDIR_RND);

	nexpr = ap_texpr1_binop(AP_TEXPR_SUB,
			ap_texpr1_copy(exp2),
			ap_texpr1_copy(exp1),
			ap_type,
			AP_RDIR_RND);


	switch (inst->getPredicate()) {
		case CmpInst::FCMP_FALSE:
		case CmpInst::FCMP_OEQ: 
		case CmpInst::FCMP_UEQ: 
		case CmpInst::ICMP_EQ:
			constyp = AP_CONS_EQ; // equality constraint
			nconstyp = AP_CONS_DISEQ;
			break;
		case CmpInst::FCMP_OGT:
		case CmpInst::FCMP_UGT:
		case CmpInst::ICMP_UGT: 
		case CmpInst::ICMP_SGT: 
			constyp = AP_CONS_SUP; // > constraint
			nconstyp = AP_CONS_SUPEQ;
			break;
		case CmpInst::FCMP_OLT: 
		case CmpInst::FCMP_ULT: 
		case CmpInst::ICMP_ULT: 
		case CmpInst::ICMP_SLT: 
			swap = expr;
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
			swap = expr;
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
	}
	if (result) {
		// creating the TRUE constraints
		create_constraints(constyp,expr,nexpr,cons);
	} else {
		// creating the FALSE constraints
		create_constraints(nconstyp,nexpr,expr,cons);
	}
}

void AI::computeConstantCondition(	ConstantInt * inst, 
		bool result,
		std::vector<ap_tcons1_array_t*> * cons) {

		if (result) {
			// always true
			return;
		}

		// we create a unsat constraint
		// such as one of the successor is unreachable
		ap_tcons1_t tcons;
		ap_tcons1_array_t * consarray;
		ap_environment_t * env = ap_environment_alloc_empty();
		consarray = new ap_tcons1_array_t();
		tcons = ap_tcons1_make(
				AP_CONS_SUP,
				ap_texpr1_cst_scalar_double(env,1.),
				ap_scalar_alloc_set_double(0.));
		*consarray = ap_tcons1_array_make(tcons.env,1);
		ap_tcons1_array_set(consarray,0,&tcons);
		
		if (inst->isZero()) {
			// condition is always false
			cons->push_back(consarray);
		}
}


void AI::visitBranchInst (BranchInst &I){
	//*Out << "BranchInst\n" << I << "\n";	
	bool test;

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

	std::vector<ap_tcons1_array_t*> * cons = new std::vector<ap_tcons1_array_t*>();

	CmpInst * cmp = dyn_cast<CmpInst>(I.getOperand(0));
	if (cmp == NULL) {
		if (ConstantInt * c = dyn_cast<ConstantInt>(I.getOperand(0))) {
			computeConstantCondition(c,test,cons);
		} else {
			// here, we loose precision, because I.getOperand(0) could also be a
			// boolean PHI-variable
			return;
		}
	} else {
		ap_texpr_rtype_t ap_type;
		if (get_ap_type(cmp->getOperand(0), ap_type)) return;
		computeCondition(cmp,test,cons);
	}

	// we add cons in the set of constraints of the path
	constraints.push_back(cons);
}


/// for the moment, we only create the constraint for the default branch of the
/// switch. The other branches lose information...
/// This code is dead since we use the LowerSwitch pass before doing abstract
/// interpretation.
void AI::visitSwitchInst (SwitchInst &I){
	//*Out << "SwitchInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitIndirectBrInst (IndirectBrInst &I){
	//*Out << "IndirectBrInst\n" << I << "\n";	
}

void AI::visitInvokeInst (InvokeInst &I){
	//*Out << "InvokeInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitUnwindInst (UnwindInst &I){
	//*Out << "UnwindInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitUnreachableInst (UnreachableInst &I){
	//*Out << "UnreachableInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitICmpInst (ICmpInst &I){
	//*Out << "ICmpInst\n" << I << "\n";	
	//visitInstAndAddVarIfNecessary(I);
}

void AI::visitFCmpInst (FCmpInst &I){
	//*Out << "FCmpInst\n" << I << "\n";	
	//visitInstAndAddVarIfNecessary(I);
}

void AI::visitAllocaInst (AllocaInst &I){
	//*Out << "AllocaInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitLoadInst (LoadInst &I){
	//*Out << "LoadInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitStoreInst (StoreInst &I){
	//*Out << "StoreInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitGetElementPtrInst (GetElementPtrInst &I){
	//*Out << "GetElementPtrInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitPHINode (PHINode &I){
	Node * n = Nodes[focuspath.back()];
	//*Out << "PHINode\n" << I << "\n";
	// we only consider one single predecessor: the predecessor from the path
	BasicBlock * pred = focuspath[focusblock-1];

	Value * pv;
	Node * nb;

	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)&I, ap_type)) return;
	*Out << I << "\n";

	for (unsigned i = 0; i < I.getNumIncomingValues(); i++) {
		if (pred == I.getIncomingBlock(i)) {
			pv = I.getIncomingValue(i);
			nb = Nodes[I.getIncomingBlock(i)];
			ap_texpr1_t * expr = get_ap_expr(n,pv);

			if (focusblock == focuspath.size()-1) {
				n->add_var(&I);
				PHIvars.name.push_back((ap_var_t)&I);
				PHIvars.expr.push_back(*expr);
				*Out << I << " is equal to ";
				texpr1_print(expr);
				*Out << "\n";
			} else {
				set_ap_expr(&I,expr);
				ap_environment_t * env = expr->env;
				insert_env_vars_into_node_vars(env,n,(Value*)&I);
			}
		}
	}
}

void AI::visitTruncInst (TruncInst &I){
	//*Out << "TruncInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitZExtInst (ZExtInst &I){
	//*Out << "ZExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitSExtInst (SExtInst &I){
	//*Out << "SExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPTruncInst (FPTruncInst &I){
	//*Out << "FPTruncInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPExtInst (FPExtInst &I){
	//*Out << "FPExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPToUIInst (FPToUIInst &I){
	//*Out << "FPToUIInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPToSIInst (FPToSIInst &I){
	//*Out << "FPToSIInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitUIToFPInst (UIToFPInst &I){
	//*Out << "UIToFPInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitSIToFPInst (SIToFPInst &I){
	//*Out << "SIToFPInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitPtrToIntInst (PtrToIntInst &I){
	//*Out << "PtrToIntInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitIntToPtrInst (IntToPtrInst &I){
	//*Out << "IntToPtrInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitBitCastInst (BitCastInst &I){
	//*Out << "BitCastInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitSelectInst (SelectInst &I){
	//*Out << "SelectInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

// a call instruction is treated as follow : 
// we consider that the function call doesn't modify the value of the different
// variable, and we the result returned by the function is a new variable of
// type int or float, depending on the return type
void AI::visitCallInst(CallInst &I){
	//*Out << "CallInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitVAArgInst (VAArgInst &I){
	//*Out << "VAArgInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitExtractElementInst (ExtractElementInst &I){
	//*Out << "ExtractElementInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitInsertElementInst (InsertElementInst &I){
	//*Out << "InsertElementInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitShuffleVectorInst (ShuffleVectorInst &I){
	//*Out << "ShuffleVectorInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitExtractValueInst (ExtractValueInst &I){
	//*Out << "ExtractValueInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitInsertValueInst (InsertValueInst &I){
	//*Out << "InsertValueInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitTerminatorInst (TerminatorInst &I){
	//*Out << "TerminatorInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitBinaryOperator (BinaryOperator &I){
	Node * n = Nodes[focuspath.back()];

	//*Out << "BinaryOperator\n" << I << "\n";	
	ap_texpr_op_t op;
	switch(I.getOpcode()) {
		// Standard binary operators...
		case Instruction::Add : 
		case Instruction::FAdd: 
			op = AP_TEXPR_ADD;
			break;
		case Instruction::Sub : 
		case Instruction::FSub: 
			op = AP_TEXPR_SUB;
			break;
		case Instruction::Mul : 
		case Instruction::FMul: 
			op = AP_TEXPR_MUL;
			break;
		case Instruction::UDiv: 
		case Instruction::SDiv: 
		case Instruction::FDiv: 
			op = AP_TEXPR_DIV;
			break;
		case Instruction::URem: 
		case Instruction::SRem: 
		case Instruction::FRem: 
			op = AP_TEXPR_MOD;
			break;
			// Logical operators
		case Instruction::Shl : // Shift left  (logical)
		case Instruction::LShr: // Shift right (logical)
		case Instruction::AShr: // Shift right (arithmetic)
		case Instruction::And :
		case Instruction::Or  :
		case Instruction::Xor :
		case Instruction::BinaryOpsEnd:
			// NOT IMPLEMENTED
			return;
	}
	ap_texpr_rtype_t type;
	get_ap_type(&I,type);
	ap_texpr_rdir_t dir = AP_RDIR_RND;
	Value * op1 = I.getOperand(0);
	Value * op2 = I.getOperand(1);

	ap_texpr1_t * exp1 = get_ap_expr(n,op1);
	ap_texpr1_t * exp2 = get_ap_expr(n,op2);
	common_environment(&exp1,&exp2);

	// we create the expression associated to the binary op
	ap_texpr1_t * exp = ap_texpr1_binop(op,exp1, exp2, type, dir);
	set_ap_expr(&I,exp);

	// this value may use some apron variables 
	// we add these variables in the Node's variable structure, such that we
	// remember that instruction I uses these variables
	//
	ap_environment_t * env = exp->env;
	insert_env_vars_into_node_vars(env,n,(Value*)&I);
}

void AI::visitCmpInst (CmpInst &I){
	//*Out << "CmpInst\n" << I << "\n";	
	//visitInstAndAddVarIfNecessary(I);
}

void AI::visitCastInst (CastInst &I){
	//*Out << "CastInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}


void AI::visitInstAndAddVarIfNecessary(Instruction &I) {
	Node * n = Nodes[focuspath.back()];
	ap_environment_t* env = NULL;
	ap_var_t var = (Value *) &I; 

	ap_texpr_rtype_t ap_type;
	
	if (get_ap_type((Value*)&I, ap_type)) return;

	if (!LV->isLiveThroughBlock(&I,I.getParent()) 
		&& !LV->isUsedInBlock(&I,I.getParent()))
		return;

	if (ap_type == AP_RTYPE_INT) { 
		env = ap_environment_alloc(&var,1,NULL,0);
	} else {
		env = ap_environment_alloc(NULL,0,&var,1);
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	set_ap_expr(&I,exp);
	//print_texpr(exp);
	n->add_var((Value*)var);
}
