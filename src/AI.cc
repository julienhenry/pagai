#include <vector>

#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LiveValues.h"

#include "llvm/Analysis/Passes.h"

#include "ap_global1.h"
#include "pk.h"

#include "AI.h"
#include "Expr.h"
#include "Node.h"
#include "apron.h"
#include "InitVerif.h"

using namespace llvm;

char AI::ID = 0;

const char * AI::getPassName() const {
	return "AI";
}

void AI::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<LiveValues>();
	AU.addRequired<initVerif>();
}

bool AI::runOnModule(Module &M) {
	Function * F = M.getFunction("main");

	man = pk_manager_alloc(true);
	if (F == NULL) {
		fouts() << "main function not found\n";
	} else {
		fouts() << "main function found\n";
		computeFunction(F);
	}
	ap_manager_free(man);
	return 0;
}

void AI::computeFunction(Function * F) {
	BasicBlock * b;
	Node * n;

	/*A = {first basicblock} */
	b = F->begin();
	n = Nodes[b];
	A.push(n);

	/* get the information about live variables from the LiveValues pass*/
	LV = &(getAnalysis<LiveValues>(*F));
	/* add all function's arguments into the environment of the first bb */
	for (Function::arg_iterator a = F->arg_begin(), e = F->arg_end(); a != e; ++a) {
		Argument * arg = a;
		if (!(arg->use_empty())) {
			n->add_var(arg);
		} else {
			fouts() << "argument " << *a << " never used !\n";
		}
	}
	/* first abstract value is top */
	ap_environment_t * env = n->create_env();
	n->X = new ap_abstract1_t(ap_abstract1_top(man,env));

	/* Simple Abstract Interpretation algorithm */
	while (!A.empty()) {
		n = A.top();
		A.pop();
		computeNode(n);
	}
}

void AI::computeNode(Node * n) {
	BasicBlock * b = n->bb;
	Node * pred;
	ap_abstract1_t Xtemp;
	ap_environment_t * env;
	bool update = false;

	std::vector<ap_abstract1_t> X_pred;

	fouts() << "#######################################################\n";
	fouts() << "Computing node:\n";
	fouts() << *b << "\n";
	fouts() << "-------------------------------------------------------\n";

	n->phi_vars.clear();
	n->intVar.clear();
	n->realVar.clear();

	/* visit instructions */
	for (BasicBlock::iterator i = b->begin(), e = b->end();
			i != e; ++i) {
		visit(*i);
	}

	/* creation of the polyhedron at the beginning of the basicblock 
	 *
	 * compute the polyhedra associated to each predecessors
	 */
	ap_abstract1_t X; 
	for (pred_iterator p = pred_begin(b), E = pred_end(b); p != E; ++p) {
		BasicBlock *pb = *p;
		pred = Nodes[pb];

		if (pred->X != NULL) {
			
			n->intVar.insert(pred->intVar.begin(),pred->intVar.end());
			n->realVar.insert(pred->realVar.begin(),pred->realVar.end());

			X = ap_abstract1_copy(man,pred->X);

			env = n->create_env();
			X = ap_abstract1_change_environment(man,true,&X,env,false);

			/* intersect with the transition's condition */
			if (pred->tcons.count(n) && pred->tcons[n] != NULL)
				X = ap_abstract1_meet_tcons_array(man,true,&X,pred->tcons[n]);
			/* we still need to add phi variables into our domain 
			 * and assign them to the right value
			 */
			X = ap_abstract1_assign_texpr_array(man,true,&X,
					&n->phi_vars[pred].name[0],
					&n->phi_vars[pred].expr[0],
					n->phi_vars[pred].name.size(),
					NULL);

			X_pred.push_back(ap_abstract1_copy(man,&X));
		}
	}

	/* Xtemp is the join of all predecessors */
	env = n->create_env();

	if (X_pred.size() > 0) {
		ap_abstract1_t  Xpreds[X_pred.size()];
		for (int i=0; i < X_pred.size(); i++) {
			Xpreds[i] = ap_abstract1_change_environment(man,false,&X_pred[i],env,false);
		}
		Xtemp = ap_abstract1_join_array(man,Xpreds,X_pred.size());	
		Xtemp = ap_abstract1_change_environment(man,false,&Xtemp,env,false);
	} else {
		/* we are in the first basicblock of the function */
		Xtemp = ap_abstract1_bottom(man,env);
		update = true;
	}

	if (n->X == NULL) {
		n->X = (ap_abstract1_t*)malloc(sizeof(ap_abstract1_t));
		*(n->X) = Xtemp;
		update = true;
	} else {
		/* environment may be bigger since the last computation of this node */
		if (!ap_environment_is_eq(env,n->X->env)) {
			*(n->X) = ap_abstract1_change_environment(man,false,n->X,env,false);
			//update = true;
		}
		/* update the abstract value if it is bigger than the previous one */
		if (!ap_abstract1_is_leq(man,&Xtemp,n->X)) {
			n->X = (ap_abstract1_t*)malloc(sizeof(ap_abstract1_t));
			*(n->X) = Xtemp;
			update = true;
		}
	}
	if (update) {
		/* update the successors of n */
		for (succ_iterator s = succ_begin(b), E = succ_end(b); s != E; ++s) {
			BasicBlock *sb = *s;
			A.push(Nodes[sb]);
		}
	}
	ap_abstract1_fprint(stdout,man,n->X);
}

void AI::visitReturnInst (ReturnInst &I){
	fouts() << "returnInst\n" << I << "\n";
}

void AI::computeCondition(	CmpInst * inst, 
		ap_tcons1_array_t * true_cons, 
		ap_tcons1_array_t * false_cons) {
	true_cons = NULL;
	false_cons = NULL;

}

void AI::visitBranchInst (BranchInst &I){
	Node * n = Nodes[I.getParent()];
	Node * iftrue;
	Node * iffalse;

	fouts() << "BranchInst\n" << I << "\n";	
	if (I.isUnconditional()) {
		/* no constraints */
		return;
	}
	/* branch under condition */
	iftrue = Nodes[I.getSuccessor(0)];
	iffalse = Nodes[I.getSuccessor(1)];
	ap_tcons1_array_t * true_cons;
	ap_tcons1_array_t * false_cons;
	computeCondition(dyn_cast<CmpInst>(I.getOperand(0)),true_cons,false_cons);
	/* insert into the tcons array of the node */
	n->tcons[iftrue] = true_cons;
	n->tcons[iffalse] = false_cons;
}

void AI::visitSwitchInst (SwitchInst &I){
	fouts() << "SwitchInst\n" << I << "\n";	
}

void AI::visitIndirectBrInst (IndirectBrInst &I){
	fouts() << "IndirectBrInst\n" << I << "\n";	
}

void AI::visitInvokeInst (InvokeInst &I){
	fouts() << "InvokeInst\n" << I << "\n";	
}

void AI::visitUnwindInst (UnwindInst &I){
	fouts() << "UnwindInst\n" << I << "\n";	
}

void AI::visitUnreachableInst (UnreachableInst &I){
	fouts() << "UnreachableInst\n" << I << "\n";	
}

void AI::visitICmpInst (ICmpInst &I){
	fouts() << "ICmpInst\n" << I << "\n";	
}

void AI::visitFCmpInst (FCmpInst &I){
	fouts() << "FCmpInst\n" << I << "\n";	
}

void AI::visitAllocaInst (AllocaInst &I){
	fouts() << "AllocaInst\n" << I << "\n";	
}

void AI::visitLoadInst (LoadInst &I){
	fouts() << "LoadInst\n" << I << "\n";	
}

void AI::visitStoreInst (StoreInst &I){
	fouts() << "StoreInst\n" << I << "\n";	
}

void AI::visitGetElementPtrInst (GetElementPtrInst &I){
	fouts() << "GetElementPtrInst\n" << I << "\n";	
}

void AI::visitPHINode (PHINode &I){
	ap_var_t var = (Value *) &I; 
	BasicBlock * b = I.getParent();
	Node * n = Nodes[b];

	fouts() << "PHINode\n" << I << "\n";
	n->add_var((Value*)var);
	
	for (int i = 0; i < I.getNumIncomingValues(); i++) {
		Value * pv = I.getIncomingValue(i);
		Node * nb = Nodes[I.getIncomingBlock(i)];
		if (nb->X != NULL){
		//if (nb->X != NULL && !ap_abstract1_is_bottom(man,nb->X)) {
			n->phi_vars[nb].name.push_back(var);
			n->phi_vars[nb].expr.push_back(*Expr::get_ap_expr(n,pv));
		}
	}
}

void AI::visitTruncInst (TruncInst &I){
	fouts() << "TruncInst\n" << I << "\n";	
}

void AI::visitZExtInst (ZExtInst &I){
	fouts() << "ZExtInst\n" << I << "\n";	
}

void AI::visitSExtInst (SExtInst &I){
	fouts() << "SExtInst\n" << I << "\n";	
}

void AI::visitFPTruncInst (FPTruncInst &I){
	fouts() << "FPTruncInst\n" << I << "\n";	
}

void AI::visitFPExtInst (FPExtInst &I){
	fouts() << "FPExtInst\n" << I << "\n";	
}

void AI::visitFPToUIInst (FPToUIInst &I){
	fouts() << "FPToUIInst\n" << I << "\n";	
}

void AI::visitFPToSIInst (FPToSIInst &I){
	fouts() << "FPToSIInst\n" << I << "\n";	
}

void AI::visitUIToFPInst (UIToFPInst &I){
	fouts() << "UIToFPInst\n" << I << "\n";	
}

void AI::visitSIToFPInst (SIToFPInst &I){
	fouts() << "SIToFPInst\n" << I << "\n";	
}

void AI::visitPtrToIntInst (PtrToIntInst &I){
	fouts() << "PtrToIntInst\n" << I << "\n";	
}

void AI::visitIntToPtrInst (IntToPtrInst &I){
	fouts() << "IntToPtrInst\n" << I << "\n";	
}

void AI::visitBitCastInst (BitCastInst &I){
	fouts() << "BitCastInst\n" << I << "\n";	
}

void AI::visitSelectInst (SelectInst &I){
	fouts() << "SelectInst\n" << I << "\n";	
}

void AI::visitCallInst(CallInst &I){
	Node * n = Nodes[I.getParent()];

	fouts() << "CallInst\n" << I << "\n";	
	ap_var_t var = (Value *) &I; 
	ap_environment_t* env = ap_environment_alloc(&var,1,NULL,0);
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	Expr::set_ap_expr(&I,exp);
	//print_texpr(exp);
	n->add_var((Value*)var);
}

void AI::visitVAArgInst (VAArgInst &I){
	fouts() << "VAArgInst\n" << I << "\n";	
}

void AI::visitExtractElementInst (ExtractElementInst &I){
	fouts() << "ExtractElementInst\n" << I << "\n";	
}

void AI::visitInsertElementInst (InsertElementInst &I){
	fouts() << "InsertElementInst\n" << I << "\n";	
}

void AI::visitShuffleVectorInst (ShuffleVectorInst &I){
	fouts() << "ShuffleVectorInst\n" << I << "\n";	
}

void AI::visitExtractValueInst (ExtractValueInst &I){
	fouts() << "ExtractValueInst\n" << I << "\n";	
}

void AI::visitInsertValueInst (InsertValueInst &I){
	fouts() << "InsertValueInst\n" << I << "\n";	
}

void AI::visitTerminatorInst (TerminatorInst &I){
	fouts() << "TerminatorInst\n" << I << "\n";	
}

void AI::visitBinaryOperator (BinaryOperator &I){
	Node * n = Nodes[I.getParent()];

	fouts() << "BinaryOperator\n" << I << "\n";	
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
	ap_texpr_rtype_t type = Expr::get_ap_type(&I);
	ap_texpr_rdir_t dir = AP_RDIR_NEAREST;
	Value * op1 = I.getOperand(0);
	Value * op2 = I.getOperand(1);

	ap_texpr1_t * exp1 = Expr::get_ap_expr(n,op1);
	ap_texpr1_t * exp2 = Expr::get_ap_expr(n,op2);

	/* we compute the least common environment for the two expressions */
	ap_dimchange_t * dimchange1;
	ap_dimchange_t * dimchange2;
	ap_environment_t* lcenv = ap_environment_lce(
			exp1->env,
			exp2->env,
			&dimchange1,
			&dimchange2);
	/* we extend the environments such that both expressions have the same one */
	exp1 = ap_texpr1_extend_environment(exp1,lcenv);
	exp2 = ap_texpr1_extend_environment(exp2,lcenv);
	/* we create the expression associated to the binary op */
	ap_texpr1_t * exp = ap_texpr1_binop(op,exp1, exp2, type, dir);
	Expr::set_ap_expr(&I,exp);
}

void AI::visitCmpInst (CmpInst &I){
	fouts() << "CmpInst\n" << I << "\n";	
}

void AI::visitCastInst (CastInst &I){
	fouts() << "CastInst\n" << I << "\n";	
}


