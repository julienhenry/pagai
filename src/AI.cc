#include <vector>
#include <list>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LiveValues.h"
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

using namespace llvm;

char AI::ID = 0;

static RegisterPass<AI> X("AI", "Abstract Interpretation Pass", false, true);

const char * AI::getPassName() const {
	return "AI";
}

void AI::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<Live>();
	AU.addRequired<SMT>();
	AU.addRequired<LoopInfo>();
}

bool AI::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		initFunction(F);
		computeFunction(F);
	}

	std::map<BasicBlock*,Node*>::iterator it;
	for ( it=Nodes.begin() ; it != Nodes.end(); it++ ) {
		b = it->first;
		n = Nodes[b];
		fouts() << "RESULTS ----------------------------------" << *b;
		fouts().flush();
		ap_environment_fdump(stdout,n->X->main->env);
		ap_abstract1_fprint(stdout,man,n->X->main);
		fflush(stdout);
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
	fouts().flush();
}

void AI::printBasicBlock(BasicBlock* b) {
	Node * n = Nodes[b];
	LoopInfo &LI = getAnalysis<LoopInfo>(*(b->getParent()));
	if (LI.isLoopHeader(b)) {
		fouts() << b << ": SCC=" << n->sccId << ": LOOP HEAD" << *b;
	} else {
		fouts() << b << ": SCC=" << n->sccId << ":" << *b;
	}
}

void AI::computeFunction(Function * F) {
	BasicBlock * b;
	Node * n;

	// A = {first basicblock}
	b = F->begin();
	if (b == F->end()) return;
	n = Nodes[b];

	// get the information about live variables from the LiveValues pass
	LV = &(getAnalysis<Live>(*F));
	LI = &(getAnalysis<LoopInfo>(*F));
	LSMT = &(getAnalysis<SMT>(*F));
	
	//LSMT->getPr(*F);
	//LSMT->getRho(*F);
	// add all function's arguments into the environment of the first bb
	for (Function::arg_iterator a = F->arg_begin(), e = F->arg_end(); a != e; ++a) {
		Argument * arg = a;
		if (!(arg->use_empty()))
			n->add_var(arg);
		else 
			fouts() << "argument " << *a << " never used !\n";
	}
	// first abstract value is top
	ap_environment_t * env = NULL;
	n->create_env(&env);
	n->X->set_top(env);
	A.push(n);

	// Simple Abstract Interpretation algorithm
	while (!A.empty()) {
		n = A.top();
		A.pop();
		computeNode(n);
	}
}

void AI::computeEnv(Node * n) {
	BasicBlock * b = n->bb;
	Node * pred = NULL;
	std::map<ap_var_t,std::set<Value*> >::iterator i, e;
	std::set<Value*>::iterator it, et;

	// we erase all previous elements from the maps
	n->phi_vars.clear();
	n->intVar.clear();
	n->realVar.clear();

	// visit instructions
	for (BasicBlock::iterator i = b->begin(), e = b->end();
			i != e; ++i) {
		visit(*i);
	}

	pred_iterator p = pred_begin(b), E = pred_end(b);
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
					n->intVar[(*i).first].insert(S.begin(),S.end());
			}

			for (i = pred->realVar.begin(), e = pred->realVar.end(); i != e; ++i) {
				std::set<Value*> S;
				for (it = (*i).second.begin(), et = (*i).second.end(); it != et; ++it) {
					Value * v = *it;
					if (LV->isLiveThroughBlock(v,b)) 
						S.insert(v);
				}
				if (S.size()>0)
					n->realVar[(*i).first].insert(S.begin(),S.end());
			}
		}
	}
}

// This function is only used by AI::ComputeHull 
void assign_phivars_and_push(
		ap_manager_t * man, 
		Abstract * X, 
		Node * n, 
		Node * pred, 
		std::vector<Abstract*> * X_pred) {
	// we still need to add phi variables into our domain 
	// and assign them the the right value
	X->assign_texpr_array(
			&n->phi_vars[pred].name[0],
			&n->phi_vars[pred].expr[0],
			n->phi_vars[pred].name.size(),
			NULL);

	X->canonicalize();
	
	// the created abstract domain could be at bottom
	// In that case, we don't isert its value in the vector
	if (!ap_abstract1_is_bottom(man,X->main))
		X_pred->push_back(X);
	else
		delete X;
}

void AI::computeHull(
		ap_environment_t * env, 
		Node * n, 
		Abstract &Xtemp, 
		bool &update) {
	std::vector<Abstract*> X_pred;
	Abstract * X;
	BasicBlock * b = n->bb;
	Node * pred = NULL;

	pred_iterator p = pred_begin(b), E = pred_end(b);
	if (p == E) {
		// we are in the first basicblock of the function
		update = true;
	}

	for (; p != E; ++p) {
		BasicBlock *pb = *p;
		pred = Nodes[pb];
		
		// we only take pred as a predecessor if its main value is not bottom
		if (pred->X->main != NULL && !ap_abstract1_is_bottom(man,pred->X->main)) {


			X = new Abstract(pred->X);

			// we transform the abstract domain such that it fits the
			// environment of the new one.
			X->change_environment(env);

			// intersect with the transition's condition
			if (pred->tcons.count(n)) {
				std::vector<ap_tcons1_array_t*>::iterator i, e;
				for (i = pred->tcons[n].begin(), e = pred->tcons[n].end(); i != e; i++) {
					Abstract * X2 = new Abstract(X);
					X2->meet_tcons_array(*i);
					// we still have to assign the right values to phi
					// variables, and push X2 in X_pred if it is not bottom
					assign_phivars_and_push(man,X2,n,pred,&X_pred);
				}
				delete X;
			} else {
				// we still have to assign the right values to phi
				// variables, and push X in X_pred if it is not bottom
				assign_phivars_and_push(man,X,n,pred,&X_pred);
			}
		}
	}
	
	DEBUG(
	for (std::map<ap_var_t,std::set<Value*> >::iterator i = n->intVar.begin(), 
			e = n->intVar.end();i != e; ++i) {
		Value * v = (Value *) (*i).first;	
		if (LV->isLiveThroughBlock(v,b)) {
			fouts() << "Value is Live :" << *v << "\n";
		} else {
			fouts() << "Value is NOT Live :" << *v << "\n";
		}
	}
	);

	// Xtemp is the join of all predecessors we just computed

	if (X_pred.size() > 0) {
		Xtemp.join_array(env,X_pred);
	} else {
		// we are in the first basicblock of the function
		// or an unreachable block
		Xtemp.set_bottom(env);
	}
}

void AI::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Abstract * Xtemp;
	bool update = false;

	if (is_computed.count(n) && is_computed[n]) {
		return;
	}
	
	DEBUG (
	fouts() << "#######################################################\n";
	fouts() << "Computing node: " << b << "\n";
	fouts() << *b << "\n";
	);

	is_computed[n] = true;

	// compute the environement we will use to create our abstract domain
	computeEnv(n);
	n->create_env(&n->env);
	Xtemp = new Abstract(man,n->env);
	// computing the new abstract value, by doing the convex hull of all
	// predecessors
	computeHull(n->env,n,*Xtemp,update);

	// environment may be bigger since the last computation of this node
	// indeed, there may be some Phi-vars with more than 1 possible incoming
	// edge, whereas only one possible incoming edge was possible before

	// we compute the set of variable that have to be added in the environment
	std::set<ap_var_t> Vars;
	for (unsigned i = 0; i < n->env->intdim + n->env->realdim; i++) {
		Vars.insert(ap_environment_var_of_dim(n->env,i));
	}
	for (unsigned i = 0; i < n->X->main->env->intdim + n->X->main->env->realdim; i++) {
		Vars.erase(ap_environment_var_of_dim(n->X->main->env,i));
	}
	n->X->change_environment(n->env);

	// Vars contains the new variables.
	// For each of these variables, we associate the right expression in the
	// abstract
	ap_texpr1_t * expr = NULL;
	ap_var_t var;
	std::vector<ap_var_t> Names;
	std::vector<ap_texpr1_t> Exprs;

	std::set<ap_var_t>::iterator i = Vars.begin(), e = Vars.end();
	if (i != e)
		update = true;


	for (; i != e; i++) {
		var = *i;
		DEBUG(fouts() << "value " << *(Value*)var <<  " is added\n";)

		// we get the previous definition of the expression
			expr = get_phivar_previous_expr((Value*)var);

		if (expr != NULL) {
			//ap_environment_fdump(stdout,n->env);
			//ap_environment_fdump(stdout,expr->env);
			expr = ap_texpr1_extend_environment(expr,n->env);

			// Here, there is a problem !
			if (expr != NULL) {
				Names.push_back(var);
				Exprs.push_back(*expr);
			}
		}
	}
	if (Names.size())
		n->X->assign_texpr_array(
				&Names[0],
				&Exprs[0],
				Names.size(),
				NULL);

	//fouts() << "n->X:\n";
	//n->X->print();
	//fouts() << "Xtemp:\n";
	//Xtemp->print();
	
	// if it is a loop header, then widening 
	if (LI->isLoopHeader(b)) {
		Xtemp->widening(n);
	}

	// update the abstract value if it is bigger than the previous one
	if ( !Xtemp->is_leq(n->X)) {
		delete n->X;
		n->X = new Abstract(Xtemp);
		update = true;
	}
	delete Xtemp;

	if (update) {
		// update the successors of n 
		for (succ_iterator s = succ_begin(b), E = succ_end(b); s != E; ++s) {
			BasicBlock *sb = *s;
			A.push(Nodes[sb]);
			is_computed[Nodes[sb]] = false;
		}
	}
	DEBUG(
	fouts() << "RESULT:\n";
	fouts().flush();
	n->X->print();
	);
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
	//fouts() << "returnInst\n" << I << "\n";
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
		std::vector<ap_tcons1_array_t*> * true_cons, 
		std::vector<ap_tcons1_array_t *> * false_cons) {

	Node * n = Nodes[inst->getParent()];
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
		case CmpInst::FCMP_TRUE: 
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
			constyp = AP_CONS_DISEQ; // disequality constraint
			nconstyp = AP_CONS_EQ;
			break;
		case CmpInst::FCMP_ORD: 
		case CmpInst::FCMP_UNO:
		case CmpInst::BAD_ICMP_PREDICATE:
		case CmpInst::BAD_FCMP_PREDICATE:
			fouts() << "ERROR : Unknown predicate\n";
			break;
	}
	// creating the TRUE constraints
	create_constraints(constyp,expr,nexpr,true_cons);

	// creating the FALSE constraints
	create_constraints(nconstyp,nexpr,expr,false_cons);
}

void AI::computeConstantCondition(	ConstantInt * inst, 
		std::vector<ap_tcons1_array_t*> * true_cons, 
		std::vector<ap_tcons1_array_t *> * false_cons) {

		// we create a unsat constraint
		// such as one of the successor is unreachable
		ap_tcons1_t cons;
		ap_tcons1_array_t * consarray;
		ap_environment_t * env = ap_environment_alloc_empty();
		consarray = new ap_tcons1_array_t();
		cons = ap_tcons1_make(
				AP_CONS_SUP,
				ap_texpr1_cst_scalar_double(env,1.),
				ap_scalar_alloc_set_double(0.));
		*consarray = ap_tcons1_array_make(cons.env,1);
		ap_tcons1_array_set(consarray,0,&cons);
		
		if (inst->isZero()) {
			// condition is always false
			true_cons->push_back(consarray);
		} else {
			// condition is always true
			false_cons->push_back(consarray);
		}
}


void AI::visitBranchInst (BranchInst &I){
	//fouts() << "BranchInst\n" << I << "\n";	
	Node * n = Nodes[I.getParent()];
	Node * iftrue;
	Node * iffalse;

	//fouts() << "BranchInst\n" << I << "\n";	
	if (I.isUnconditional()) {
		/* no constraints */
		return;
	}
	// branch under condition
	iftrue = Nodes[I.getSuccessor(0)];
	iffalse = Nodes[I.getSuccessor(1)];
	std::vector<ap_tcons1_array_t*> * true_cons = new std::vector<ap_tcons1_array_t*>();
	std::vector<ap_tcons1_array_t*> * false_cons = new std::vector<ap_tcons1_array_t*>();

	CmpInst * cmp = dyn_cast<CmpInst>(I.getOperand(0));
	if (cmp == NULL) {
		if (ConstantInt * c = dyn_cast<ConstantInt>(I.getOperand(0))) {
			computeConstantCondition(c,true_cons,false_cons);
		} else {
			// here, we loose precision, because I.getOperand(0) could also be a
			// boolean PHI-variable
			return;
		}
	} else {
		ap_texpr_rtype_t ap_type;
		if (get_ap_type(cmp->getOperand(0), ap_type)) return;
		computeCondition(cmp,true_cons,false_cons);
	}

	// free the previous tcons
	if (n->tcons.count(iftrue)) {
		for (std::vector<ap_tcons1_array_t*>::iterator i = n->tcons[iftrue].begin(), e = n->tcons[iftrue].end(); i != e; ++i)
			ap_tcons1_array_clear(*i);
		n->tcons.erase(iftrue);
	}
	if (n->tcons.count(iffalse)) {
		for (std::vector<ap_tcons1_array_t*>::iterator i = n->tcons[iffalse].begin(), e = n->tcons[iffalse].end(); i != e; ++i)
			ap_tcons1_array_clear(*i);
		n->tcons.erase(iffalse);
	}
	// insert into the tcons array of the node
	n->tcons[iftrue] = *true_cons;
	n->tcons[iffalse] = *false_cons;
}


/// for the moment, we only create the constraint for the default branch of the
/// switch. The other branches lose information...
/// This code is dead since we use the LowerSwitch pass before doing abstract
/// interpretation.
void AI::visitSwitchInst (SwitchInst &I){
	//fouts() << "SwitchInst\n" << I << "\n";	
	Node * n = Nodes[I.getParent()];
	ap_tcons1_array_t * false_cons;
	ap_texpr1_t * expr;
	unsigned num = I.getNumCases();

	Value * Condition =	I.getCondition();
	ap_texpr1_t * ConditionExp = get_ap_expr(n,Condition);
	Node * Default = Nodes[I.getDefaultDest()];
	ap_texpr_rtype_t ap_type;
	get_ap_type(Condition,ap_type);

	false_cons = new ap_tcons1_array_t();
	*false_cons = ap_tcons1_array_make(ConditionExp->env,num);
	
	for (unsigned i = 0; i < num; i++) {
		ConstantInt * CaseValue = I.getCaseValue(i);
		ap_texpr1_t * CaseExp = get_ap_expr(n,CaseValue);

		common_environment(&ConditionExp,&CaseExp);
		
		expr = ap_texpr1_binop(AP_TEXPR_SUB,
				ap_texpr1_copy(ConditionExp),
				ap_texpr1_copy(CaseExp),
				ap_type,
				AP_RDIR_RND);

		// creating the FALSE constraint
		ap_tcons1_t cons = ap_tcons1_make(
				AP_CONS_DISEQ,
				expr,
				ap_scalar_alloc_set_double(0));
		ap_tcons1_array_set(false_cons,i,&cons);
	
		// insert into the tcons array of the node
	}
	std::vector<ap_tcons1_array_t*> v;
	v.push_back(false_cons);
	n->tcons[Default] = v;
}

void AI::visitIndirectBrInst (IndirectBrInst &I){
	//fouts() << "IndirectBrInst\n" << I << "\n";	
}

void AI::visitInvokeInst (InvokeInst &I){
	//fouts() << "InvokeInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitUnwindInst (UnwindInst &I){
	//fouts() << "UnwindInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitUnreachableInst (UnreachableInst &I){
	//fouts() << "UnreachableInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitICmpInst (ICmpInst &I){
	//fouts() << "ICmpInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFCmpInst (FCmpInst &I){
	//fouts() << "FCmpInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitAllocaInst (AllocaInst &I){
	//fouts() << "AllocaInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitLoadInst (LoadInst &I){
	//fouts() << "LoadInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitStoreInst (StoreInst &I){
	//fouts() << "StoreInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitGetElementPtrInst (GetElementPtrInst &I){
	//fouts() << "GetElementPtrInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitPHINode (PHINode &I){
	//fouts() << "PHINode\n" << I << "\n";
	ap_var_t var = (Value *) &I; 
	BasicBlock * b = I.getParent();
	Node * n = Nodes[b];
	Value * pv;
	Node * nb;
	std::list<int> IncomingValues;

	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)&I, ap_type)) return;

	set_phivar_previous_expr((Value*)&I,get_ap_expr(n,(Value*)&I));
	// determining the predecessors of the phi variable, and insert in a list
	// the predecessors that are not at bottom.
	for (unsigned i = 0; i < I.getNumIncomingValues(); i++) {
		pv = I.getIncomingValue(i);
		nb = Nodes[I.getIncomingBlock(i)];
		if (nb->X->main != NULL && !ap_abstract1_is_bottom(man,nb->X->main)) {
			IncomingValues.push_back(i);
		}
	}

	if (IncomingValues.size() == 1) {
		// only one incoming value is possible : it is useless to add a new
		// variable, since Value I is directly equal to the associated incoming
		// value
		fouts() << I << "one single incoming value\n";
		int i = IncomingValues.front();
		pv = I.getIncomingValue(i);
		nb = Nodes[I.getIncomingBlock(i)];
		ap_texpr1_t * expr = get_ap_expr(nb,pv);
		set_ap_expr(&I,expr);
		ap_environment_t * env = expr->env;

		//set_phivar_first_expr(&I,expr);
		// this instruction may use some apron variables (from the abstract
		// domain)
		// we add these variables in the Node's variable structure, such that we
		// remember that the instruction I uses these variables
		insert_env_vars_into_node_vars(env,n,(Value*)&I);
	} else {
		ap_texpr1_t * exp = get_ap_expr(n,(Value*)var);
		if (exp != NULL && !ap_texpr1_has_var(exp,var)) {
			insert_env_vars_into_node_vars(exp->env,n,(Value*)&I);
		}
		n->add_var((Value*)var);
		while (!IncomingValues.empty()) {
			int i = IncomingValues.front();
			IncomingValues.pop_front();
			pv = I.getIncomingValue(i);
			// when the value is an undef value, we don't insert it into our
			// phivar table, because undef is not live in the successors of the
			// block
			if (!isa<UndefValue>(pv)) {
				nb = Nodes[I.getIncomingBlock(i)];
				n->phi_vars[nb].name.push_back(var);
				n->phi_vars[nb].expr.push_back(*get_ap_expr(nb,pv));
			}
		}
	}
}

void AI::visitTruncInst (TruncInst &I){
	//fouts() << "TruncInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitZExtInst (ZExtInst &I){
	//fouts() << "ZExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitSExtInst (SExtInst &I){
	//fouts() << "SExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPTruncInst (FPTruncInst &I){
	//fouts() << "FPTruncInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPExtInst (FPExtInst &I){
	//fouts() << "FPExtInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPToUIInst (FPToUIInst &I){
	//fouts() << "FPToUIInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitFPToSIInst (FPToSIInst &I){
	//fouts() << "FPToSIInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitUIToFPInst (UIToFPInst &I){
	//fouts() << "UIToFPInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitSIToFPInst (SIToFPInst &I){
	//fouts() << "SIToFPInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitPtrToIntInst (PtrToIntInst &I){
	//fouts() << "PtrToIntInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitIntToPtrInst (IntToPtrInst &I){
	//fouts() << "IntToPtrInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitBitCastInst (BitCastInst &I){
	//fouts() << "BitCastInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitSelectInst (SelectInst &I){
	//fouts() << "SelectInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

// a call instruction is treated as follow : 
// we consider that the function call doesn't modify the value of the different
// variable, and we the result returned by the function is a new variable of
// type int or float, depending on the return type
void AI::visitCallInst(CallInst &I){
	//fouts() << "CallInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitVAArgInst (VAArgInst &I){
	//fouts() << "VAArgInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitExtractElementInst (ExtractElementInst &I){
	//fouts() << "ExtractElementInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitInsertElementInst (InsertElementInst &I){
	//fouts() << "InsertElementInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitShuffleVectorInst (ShuffleVectorInst &I){
	//fouts() << "ShuffleVectorInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitExtractValueInst (ExtractValueInst &I){
	//fouts() << "ExtractValueInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitInsertValueInst (InsertValueInst &I){
	//fouts() << "InsertValueInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitTerminatorInst (TerminatorInst &I){
	//fouts() << "TerminatorInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitBinaryOperator (BinaryOperator &I){
	Node * n = Nodes[I.getParent()];

	//fouts() << "BinaryOperator\n" << I << "\n";	
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
	//fouts() << "CmpInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

void AI::visitCastInst (CastInst &I){
	//fouts() << "CastInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}


void AI::visitInstAndAddVarIfNecessary(Instruction &I) {
	Node * n = Nodes[I.getParent()];
	ap_environment_t* env = NULL;
	ap_var_t var = (Value *) &I; 

	ap_texpr_rtype_t ap_type;
	
	if (get_ap_type((Value*)&I, ap_type)) return;

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
