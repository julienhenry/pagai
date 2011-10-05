#include <sstream>
#include <vector>
#include <iostream>
#include <string>

#include "llvm/Support/CFG.h"
#include "llvm/Analysis/LoopInfo.h"

#include "SMTpass.h"
#include "z3_manager.h"
#include "yices.h"
#include "Expr.h"
#include "apron.h"
#include "Debug.h"

using namespace std;

char SMTpass::ID = 0;
static RegisterPass<SMTpass>
X("SMTpass","SMT-formula creation pass",false,true);


const char * SMTpass::getPassName() const {
	return "SMT";
}

SMTpass::SMTpass() : ModulePass(ID), nundef(0) {
	switch (getSMTSolver()) {
		case Z3_MANAGER:
			man = new z3_manager();
			break;
		case YICES_MANAGER: 
			man = new yices();
			break;
	}
}

SMTpass::~SMTpass() {
	delete man;
}

void SMTpass::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfo>();
	AU.setPreservesAll();
}

bool SMTpass::runOnModule(Module &M) {
	return 0;
}

std::set<BasicBlock*>* SMTpass::getPr(Function &F) {
	if (!Pr.count(&F))
		computePr(F);
	return Pr[&F];
}

SMT_expr SMTpass::getRho(Function &F) {
	if (!rho.count(&F))
		computeRho(F);
	return rho[&F];
}

void SMTpass::reset_SMTcontext() {
	rho.clear();
	delete man;
	switch (getSMTSolver()) {
		case Z3_MANAGER:
			man = new z3_manager();
			break;
		case YICES_MANAGER: 
			man = new yices();
			break;
	}
}

SMT_expr SMTpass::texpr1ToSmt(ap_texpr1_t texpr) {
	// NOT IMPLEMENTED
	return NULL;
}

SMT_expr SMTpass::linexpr1ToSmt(BasicBlock* b, ap_linexpr1_t linexpr, bool &integer) {
	std::vector<SMT_expr> elts;
	SMT_expr val;
	SMT_expr coefficient;

	integer = false;
	double value;
	size_t i;
	ap_var_t var;
	ap_coeff_t* coeff;

	ap_linexpr1_ForeachLinterm1(&linexpr,i,var,coeff){ 
		val = getValueExpr((Value*)var, primed[b]);
		if (((Value*)var)->getType()->isIntegerTy()) {
			coefficient = scalarToSmt(coeff->val.scalar,true,value);
			if (value != 0) 
				integer = true;
		} else {
			coefficient = scalarToSmt(coeff->val.scalar,false,value);
		}
		
		if (value != 0) {
			if (value == 1) {
				elts.push_back(val);
			} else {
				std::vector<SMT_expr> elt;
				elt.push_back(val);
				elt.push_back(coefficient);
				elts.push_back(man->SMT_mk_mul(elt));
			}
		}
	}
	coeff = ap_linexpr1_cstref(&linexpr);
	coefficient = scalarToSmt(coeff->val.scalar,integer,value);
	if (value != 0)
		elts.push_back(coefficient);
	return man->SMT_mk_sum(elts);
}

SMT_expr SMTpass::scalarToSmt(ap_scalar_t * scalar, bool integer, double &value) {
	mp_rnd_t round = GMP_RNDN;
	ap_double_set_scalar(&value,scalar,round);

	if (integer)
		return man->SMT_mk_num((int)value);
	else
		return man->SMT_mk_real(value);
}

SMT_expr SMTpass::lincons1ToSmt(BasicBlock * b, ap_lincons1_t lincons) {
	ap_constyp_t* constyp = ap_lincons1_constypref(&lincons);
	ap_linexpr1_t linexpr = ap_lincons1_linexpr1ref(&lincons);
	//ap_coeff_t * coeff = ap_lincons1_cstref(&lincons);
	SMT_expr scalar_smt = NULL;
	bool integer;
	SMT_expr linexpr_smt = linexpr1ToSmt(b, linexpr, integer);
	if (integer)
		scalar_smt = man->SMT_mk_num(0);
	else
		scalar_smt = man->SMT_mk_real(0);

	switch (*constyp) {
		case AP_CONS_EQ:
			return man->SMT_mk_eq(linexpr_smt,scalar_smt);
		case AP_CONS_SUPEQ:
			return man->SMT_mk_ge(linexpr_smt,scalar_smt);
		case AP_CONS_SUP: 
			return man->SMT_mk_gt(linexpr_smt,scalar_smt);
		case AP_CONS_EQMOD:
			{
				double value;
		   		SMT_expr modulo = scalarToSmt(ap_lincons1_lincons0ref(&lincons)->scalar,true,value);
		   		assert(!(value == 0));
		   		return man->SMT_mk_eq(man->SMT_mk_rem(linexpr_smt,modulo),scalar_smt);
            }
		case AP_CONS_DISEQ:
			return man->SMT_mk_diseq(linexpr_smt,scalar_smt);
	}
	// unreachable
	return NULL;
}

SMT_expr SMTpass::tcons1ToSmt(ap_tcons1_t tcons) {
	ap_constyp_t* constyp = ap_tcons1_constypref(&tcons);
	ap_texpr1_t texpr = ap_tcons1_texpr1ref(&tcons);
	ap_scalar_t* scalar = ap_tcons1_scalarref(&tcons);
	bool integer = true;
	double value;
	SMT_expr texpr_smt = texpr1ToSmt(texpr);
	SMT_expr scalar_smt = scalarToSmt(scalar,integer,value);

	switch (*constyp) {
		case AP_CONS_EQ:
			return man->SMT_mk_eq(texpr_smt,scalar_smt);
		case AP_CONS_SUPEQ:
			return man->SMT_mk_ge(texpr_smt,scalar_smt);
		case AP_CONS_SUP: 
			return man->SMT_mk_gt(texpr_smt,scalar_smt);
		case AP_CONS_EQMOD:
			return man->SMT_mk_eq(texpr_smt,scalar_smt);
		case AP_CONS_DISEQ:
			return man->SMT_mk_diseq(texpr_smt,scalar_smt);
	}
	// unreachable
	return NULL;
}


SMT_expr SMTpass::AbstractDisjToSmt(BasicBlock * b, AbstractDisj * A, bool insert_booleans) {
	std::vector<SMT_expr> disj;
	std::vector<Abstract*>::iterator it = A->disj.begin(), et = A->disj.end();
	if (insert_booleans) {
		for (int index = 0;it != et; it++, index++) {
			std::vector<SMT_expr> cunj;
			cunj.push_back(AbstractToSmt(b,*it));
			// we create a boolean predicate for each disjunct
			SMT_var dvar = man->SMT_mk_bool_var(getDisjunctiveIndexName(A,index));
			SMT_expr dexpr = man->SMT_mk_expr_from_bool_var(dvar);
			cunj.push_back(dexpr);
			disj.push_back(man->SMT_mk_and(cunj));
		}
	} else {
		for (;it != et; it++) {
			disj.push_back(AbstractToSmt(b,*it));
		}
	}
	return man->SMT_mk_or(disj);
}

SMT_expr SMTpass::AbstractToSmt(BasicBlock * b, Abstract * A) {

	if (A->is_bottom()) return man->SMT_mk_false();
	if (AbstractDisj * Adis = dynamic_cast<AbstractDisj*>(A)) 
		return AbstractDisjToSmt(b,Adis,false);

	std::vector<SMT_expr> constraints;
	ap_lincons1_t lincons;
	ap_lincons1_array_t lincons_array = A->to_lincons_array();
	size_t n = ap_lincons1_array_size(&lincons_array);
	for (size_t i = 0; i < n; i++) {
		lincons = ap_lincons1_array_get(&lincons_array,i);
		constraints.push_back(lincons1ToSmt(b,lincons));
	}
	ap_lincons1_array_clear(&lincons_array);
	if (constraints.size() == 0)
		return man->SMT_mk_true();
	else
		return man->SMT_mk_and(constraints);
}

const std::string SMTpass::getDisjunctiveIndexName(AbstractDisj * A, int index) {
	std::ostringstream name;
	name << "d_" << A << "_" << index;
	return name.str();
}

const std::string SMTpass::getUndeterministicChoiceName(Value * v) {
	std::ostringstream name;
	name << "c_" << v;
	return name.str();
}

const std::string SMTpass::getNodeName(BasicBlock* b, bool src) {
	std::ostringstream name;
	std::set<BasicBlock*> * FPr = getPr(*(b->getParent()));
	if (FPr->count(b)) {
		if (src)
			name << "bs_";
		else
			name << "bd_";
	} else {
		name << "b_";
	}
	name << b;
	return name.str();
}

const std::string SMTpass::getEdgeName(BasicBlock* b1, BasicBlock* b2) {
	std::ostringstream name;
	name << "t_" << b1 << "_" << b2;
	return name.str();
}

const std::string SMTpass::getValueName(Value * v, bool primed) {
	std::ostringstream name;
	if (primed)
		name << "x'_" << ap_var_to_string((ap_var_t)v) << "_";
	else
		name << "x_" << ap_var_to_string((ap_var_t)v) <<  "_";
	return name.str();
}


SMT_type SMTpass::getValueType(Value * v) {
	switch (v->getType()->getTypeID()) {
		case Type::IntegerTyID:
			return man->int_type;
		default:
			return man->float_type;
	}
}

SMT_var SMTpass::getVar(Value * v, bool primed) {
	std::string name = getValueName(v,primed);
	return man->SMT_mk_var(name,getValueType(v));
}

SMT_expr SMTpass::getValueExpr(Value * v, std::set<Value*> ssa_defs) {
	SMT_var var;

	ap_texpr_rtype_t ap_type;
	if (get_ap_type(v, ap_type)) {
		// this may be a boolean
		if (ap_type == AP_RTYPE_INT) {
			// this is a boolean
			// so we can create a boolean variable
			SMT_expr cond = NULL;
			if (isa<CmpInst>(v)) {
				cond = computeCondition(dyn_cast<CmpInst>(v));
			} else if (isa<PHINode>(v)) {
				// BUG : stack overflow when two PHINode objects are mutually
				// dependent
				cond = computeCondition(dyn_cast<PHINode>(v));
			} else if (ConstantInt * vint = dyn_cast<ConstantInt>(v)) {
				if (vint->isZero()) {
					cond = man->SMT_mk_false();
				} else if (vint->isOne()) {
					cond = man->SMT_mk_true();
				}
			} 
			if (cond == NULL) {
				SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(v));
				cond = man->SMT_mk_expr_from_bool_var(cvar);
			}
			return cond;
		} else {
			*Out << "ERROR: getValueExpr returns NULL\n";
			return NULL;
		}
	}

	if (isa<ConstantInt>(v)) { 
		ConstantInt * Int = dyn_cast<ConstantInt>(v);
		int64_t n = Int->getSExtValue();
		return man->SMT_mk_num((int)n);
	} else if (isa<ConstantFP>(v)) {
		ConstantFP * FP = dyn_cast<ConstantFP>(v);
		double x = FP->getValueAPF().convertToDouble();
		if (FP->isExactlyValue(x)) {
			DEBUG(
			*Out << "getValueExpr" << *v << " (exactly " << x << ")\n";
			);
		} else {
			DEBUG(
			*Out << "getValueExpr" << *v << " (NOT exactly " << x << ")\n";
			);
			float f = FP->getValueAPF().convertToFloat(); 
			x = f;
		}
		return man->SMT_mk_real(x);
	} else if (isa<UndefValue>(v)) {
		std::ostringstream name;
		name << getValueName(v,false) << "_" << nundef;
		nundef++;
		return man->SMT_mk_expr_from_var(man->SMT_mk_var(name.str(),getValueType(v)));
	} else if (isa<Instruction>(v) || isa<Argument>(v)) {
		if (ssa_defs.count(v))
			var = getVar(v,true);
		else
			var = getVar(v,false);
		return man->SMT_mk_expr_from_var(var);
	} else {
		if (ssa_defs.count(v))
			var = getVar(v,true);
		else
			var = getVar(v,false);
		return man->SMT_mk_expr_from_var(var);
	}
	return NULL;
}

/// getElementFromString - 
void SMTpass::getElementFromString(
	std::string name,
	bool &isEdge,
	bool &isIndex,
	bool &start,
	BasicBlock * &src,
	BasicBlock * &dest,
	int &index) {

	std::string edge ("t_");
	std::string simple_node ("b_");
	std::string source_node ("bs_");
	std::string dest_node ("bd_");
	std::string disjunctive_index ("d_");
	size_t found;
	void* address;
	src = NULL;
	dest = NULL;
	start = false;

	// case 1 : this is an edge
	found = name.find(edge);
	if (found!=std::string::npos) {
		isEdge = true;
		std::string source = name.substr (2,9);
		std::string destination = name.substr (12,9);
		std::istringstream srcStream(source);
		std::istringstream destStream(destination);
		
		srcStream >> std::hex >> address;
		src = (BasicBlock*)address;
		destStream >> std::hex >> address;
		dest = (BasicBlock*)address;
		return;
	}
	isEdge = false;

	// case 2 : this is a disjunctive index
	found = name.find(disjunctive_index);
	if (found!=std::string::npos) {
		isIndex = true;
		std::string source = name.substr (2,9);
		std::string stringindex(name);
		stringindex.erase(0,12);	
		std::istringstream indexStream(stringindex);
		indexStream >> index;
		return;
	}
	isIndex = false;
	
	std::string nodename;
	// case 3 : this is a node
	found = name.find(simple_node);
	if (found==std::string::npos) {
		found = name.find(source_node);
		if (found==std::string::npos) found = name.find(dest_node);
		else start = true;
		if (found!=std::string::npos) {
			nodename = name.substr (3,9);
		}
	} else {
		nodename = name.substr (2,9);
	}

	if (found != std::string::npos) {
		std::istringstream srcStream(nodename);
		srcStream >> std::hex >> address;
		src = (BasicBlock*)address;
	}

}


// computePr - computes the set Pr of BasicBlocks
// for the moment - Pr = Pw + blocks with a ret inst
void SMTpass::computePr(Function &F) {
	std::set<BasicBlock*> * FPr = new std::set<BasicBlock*>();
	BasicBlock * b;
	LI = &(getAnalysis<LoopInfo>(F));

	FPr->insert(F.begin());

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		b = i;
		if (LI->isLoopHeader(b)) {
			FPr->insert(b);
		}

		for (BasicBlock::iterator it = b->begin(), et = b->end(); it != et; ++it) {
			if (isa<ReturnInst>(*it)) {
				FPr->insert(b);
			}
		}

	}
	Pr[&F] = FPr;
}

std::set<BasicBlock*> SMTpass::getPrPredecessors(BasicBlock * b) {
	return Pr_pred[b];
}

std::set<BasicBlock*> SMTpass::getPrSuccessors(BasicBlock * b) {
	return Pr_succ[b];
}

void SMTpass::computeRhoRec(Function &F, 
		BasicBlock * b,
		BasicBlock * dest,
		bool newPr,
		std::set<BasicBlock*> * visited) {

	bool first = (visited->count(b) > 0);
	visited->insert(b);

	if (!Pr[&F]->count(b) || newPr) {
		// we recursively construct Rho, starting from the predecessors
		BasicBlock * pred;
		for (pred_iterator p = pred_begin(b), E = pred_end(b); p != E; ++p) {
			pred = *p;
			if (!Pr[&F]->count(pred)) {
				if (!visited->count(pred)) {
					computeRhoRec(F,pred,dest,false,visited);
				}
				Pr_pred[b].insert(Pr_pred[pred].begin(),Pr_pred[pred].end());
			} else {
				Pr_pred[b].insert(pred);
			}
			//Pr_succ[pred].insert(dest);
		}
	}

	if (first) return;

	// we create a boolean reachability predicate for the basicblock
	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,false));
	// we associate it the right predicate depending on its incoming edges
	std::vector<SMT_expr> predicate;
	for (pred_iterator p = pred_begin(b), e = pred_end(b); p != e; ++p) {
		SMT_var evar = man->SMT_mk_bool_var(getEdgeName(*p,b));
		predicate.push_back(man->SMT_mk_expr_from_bool_var(evar));
	}
	SMT_expr bpredicate;

	if (predicate.size() != 0)
		bpredicate = man->SMT_mk_or(predicate);
	else
		bpredicate = man->SMT_mk_false();

	SMT_expr bvar_exp = man->SMT_mk_expr_from_bool_var(bvar);
	bvar_exp = man->SMT_mk_eq(bvar_exp,bpredicate);
	rho_components.push_back(bvar_exp);
	// we compute the transformation due to the basicblock's
	// instructions
	bvar = man->SMT_mk_bool_var(getNodeName(b,false));
	instructions.clear();

	for (BasicBlock::iterator i = b->begin(), e = b->end();
			i != e; ++i) {
		visit(*i);
	}

	if (instructions.size() > 0) {
		bpredicate = man->SMT_mk_and(instructions);
		SMT_expr bvar_exp = man->SMT_mk_expr_from_bool_var(bvar);
		bvar_exp = man->SMT_mk_not(bvar_exp);
		std::vector<SMT_expr> implies;
		implies.push_back(bvar_exp);
		implies.push_back(bpredicate);
		bvar_exp = man->SMT_mk_or(implies);
		rho_components.push_back(bvar_exp);
	}
}

void SMTpass::computeRho(Function &F) {
	std::set<BasicBlock*> visited;
	BasicBlock * b;

	rho_components.clear();
	primed[NULL].clear();
	std::set<BasicBlock*>::iterator i = Pr[&F]->begin(), e = Pr[&F]->end();
	for (;i!= e; ++i) {
		b = *i;
		computeRhoRec(F,b,b,true,&visited);
	}
	rho[&F] = man->SMT_mk_and(rho_components); 

	// Pr_pred has already been computed, but not Pr_succ
	// Now, we can easily compute Pr_succ
	i = Pr[&F]->begin();
	e = Pr[&F]->end();
	for (;i!= e; ++i) {
		b = *i;
		std::set<BasicBlock*>::iterator it = Pr_pred[b].begin(), et = Pr_pred[b].end();
		for (;it != et; it++) {
			Pr_succ[*it].insert(b);
		}
	}
}


void SMTpass::push_context() {
	man->push_context();
}

void SMTpass::pop_context() {
	man->pop_context();
}

/// createSMTformula - create the smt formula that is described in the paper
///
SMT_expr SMTpass::createSMTformula(
	BasicBlock * source, 
	bool use_X_d, 
	Techniques t,
	SMT_expr constraint) {
	Function &F = *source->getParent();
	std::vector<SMT_expr> formula;
	formula.push_back(getRho(F));

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(source,true));
	formula.push_back(man->SMT_mk_expr_from_bool_var(bvar));

	std::set<BasicBlock*>::iterator i = Pr[&F]->begin(), e = Pr[&F]->end();
	for (; i!=e; ++i) {
		if (*i != source) {
			bvar = man->SMT_mk_bool_var(getNodeName(*i,true));
			formula.push_back(man->SMT_mk_not(man->SMT_mk_expr_from_bool_var(bvar)));
		}
	}

	Abstract * A = Nodes[source]->X_s[t];
	if (AbstractDisj * Adis = dynamic_cast<AbstractDisj*>(A))
		formula.push_back(AbstractDisjToSmt(NULL,Adis,true));
	else
		formula.push_back(AbstractToSmt(NULL,A));

	std::vector<SMT_expr> Or;
	std::set<BasicBlock*> Successors = getPrSuccessors(source);
	i = Successors.begin(), e = Successors.end();
	for (; i!=e; ++i) {
		std::vector<SMT_expr> SuccExp;
		SMT_var succvar = man->SMT_mk_bool_var(getNodeName(*i,false));
		SuccExp.push_back(man->SMT_mk_expr_from_bool_var(succvar));
		
		if (use_X_d)
			SuccExp.push_back(man->SMT_mk_not(AbstractToSmt(*i,Nodes[*i]->X_d[t])));
		else
			SuccExp.push_back(man->SMT_mk_not(AbstractToSmt(*i,Nodes[*i]->X_s[t])));
		
		Or.push_back(man->SMT_mk_and(SuccExp));
	}
	// if Or is empty, that means the block has no successor => formula has to
	// be false
	if (Or.size() > 0)
		formula.push_back(man->SMT_mk_or(Or));
	else
		formula.push_back(man->SMT_mk_false());

	// if constraint argument is specified, we insert it into our formula
	if (constraint != NULL)
		formula.push_back(constraint);

	return man->SMT_mk_and(formula);
}

/// solve an SMTpass formula and computes its model in case of a 'sat'
/// formula
int SMTpass::SMTsolve(SMT_expr expr, std::list<BasicBlock*> * path) {
	int index;
	return SMTsolve(expr,path,index);
}

/// solve an SMTpass formula and computes its model in case of a 'sat'
/// formula. In the case of a pass using disjunctive invariants, index is set to
/// the associated index of the disjunct to focus on.
int SMTpass::SMTsolve(SMT_expr expr, std::list<BasicBlock*> * path, int &index) {
	std::set<std::string> true_booleans;
	std::map<BasicBlock*, BasicBlock*> succ;
	int res;
	res = man->SMT_check(expr,&true_booleans);
	if (res != 1) return res;
	bool isEdge, isIndex, start;
	BasicBlock * src;
	BasicBlock * dest;
	int disj;

	std::set<std::string>::iterator i = true_booleans.begin(), e = true_booleans.end();
	for (; i != e; ++i) {
		std::string name = *i;
		getElementFromString(name,isEdge,isIndex,start,src,dest,disj);
		if (isEdge) {
			succ[src] = dest;
		} else if (isIndex) {
			index = disj;
		} else {
			if (start)
				path->push_back(src);
		}
		Out->flush();
	}

	while (succ.count(path->back())) {
		path->push_back(succ[path->back()]);
		if (path->back() == path->front()) break;
	}
	return res;	
}

int SMTpass::SMTsolve_simple(SMT_expr expr) {
	std::set<std::string> true_booleans;
	return man->SMT_check(expr,&true_booleans);
}

void SMTpass::visitReturnInst (ReturnInst &I) {
}

SMT_expr SMTpass::computeCondition(PHINode * inst) {
	return construct_phi_ite(*inst,0,inst->getNumIncomingValues());
}

SMT_expr SMTpass::computeCondition(CmpInst * inst) {

	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)inst->getOperand(0), ap_type)) {
		// the comparison is not between integers or reals
		// we create an undeterministic choice variable
		SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(inst));
		SMT_expr cexpr = man->SMT_mk_expr_from_bool_var(cvar);
		return cexpr;
	}

	SMT_expr op1, op2;

	// that's a trick: outgoing edges from Pr states never have primed variables
	// in their branchment condition. So, we use an emptyset instead of the one
	// we should use (which is dedicated to the destination node only)
	//if (Pr[inst->getParent()->getParent()]->count(inst->getParent())) {
		std::set<Value*> emptyset;
		op1 = getValueExpr(inst->getOperand(0), emptyset);
		op2 = getValueExpr(inst->getOperand(1), emptyset);
	//} else {
	//	op1 = getValueExpr(inst->getOperand(0), primed[inst->getParent()]);
	//	op2 = getValueExpr(inst->getOperand(1), primed[inst->getParent()]);
	//}

	switch (inst->getPredicate()) {
		case CmpInst::FCMP_FALSE:
			return man->SMT_mk_false();
		case CmpInst::FCMP_TRUE: 
			return man->SMT_mk_true();
		case CmpInst::FCMP_OEQ: 
		case CmpInst::FCMP_UEQ: 
		case CmpInst::ICMP_EQ:
			return man->SMT_mk_eq(op1,op2);
		case CmpInst::FCMP_OGT:
		case CmpInst::FCMP_UGT:
		case CmpInst::ICMP_UGT: 
		case CmpInst::ICMP_SGT: 
			return man->SMT_mk_gt(op1,op2);
		case CmpInst::FCMP_OLT: 
		case CmpInst::FCMP_ULT: 
		case CmpInst::ICMP_ULT: 
		case CmpInst::ICMP_SLT: 
			return man->SMT_mk_lt(op1,op2);
		case CmpInst::FCMP_OGE:
		case CmpInst::FCMP_UGE:
		case CmpInst::ICMP_UGE:
		case CmpInst::ICMP_SGE:
			return man->SMT_mk_ge(op1,op2);
		case CmpInst::FCMP_OLE:
		case CmpInst::FCMP_ULE:
		case CmpInst::ICMP_ULE:
		case CmpInst::ICMP_SLE:
			return man->SMT_mk_le(op1,op2);
		case CmpInst::FCMP_ONE:
		case CmpInst::FCMP_UNE: 
		case CmpInst::ICMP_NE: 
			return man->SMT_mk_diseq(op1,op2);
		case CmpInst::FCMP_ORD: 
		case CmpInst::FCMP_UNO:
		case CmpInst::BAD_ICMP_PREDICATE:
		case CmpInst::BAD_FCMP_PREDICATE:
			*Out << "ERROR : Unknown predicate\n";
			return NULL;
	}
	return NULL;
}

void SMTpass::visitBranchInst (BranchInst &I) {
	BasicBlock * b = I.getParent();

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,true));
	SMT_expr bexpr = man->SMT_mk_expr_from_bool_var(bvar);
	BasicBlock * s = I.getSuccessor(0);
	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	SMT_expr eexpr = man->SMT_mk_expr_from_bool_var(evar);
	SMT_expr components_and;

	if (I.isUnconditional() || s == I.getSuccessor(1)) {
		rho_components.push_back(man->SMT_mk_eq(eexpr,bexpr));
	} else {
		std::vector<SMT_expr> components;
		SMT_expr cond;
		if (isa<CmpInst>(I.getOperand(0)))
			cond = computeCondition(dyn_cast<CmpInst>(I.getOperand(0)));
		else if (isa<PHINode>(I.getOperand(0))) {
			cond = computeCondition(dyn_cast<PHINode>(I.getOperand(0)));
		} else {
			SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(I.getOperand(0)));
			cond = man->SMT_mk_expr_from_bool_var(cvar);
		}

		components.push_back(bexpr);
		if (cond != NULL)
			components.push_back(cond);
		components_and = man->SMT_mk_and(components);
		rho_components.push_back(man->SMT_mk_eq(eexpr,components_and));

		components.clear();
		cond = man->SMT_mk_not(cond);
		s = I.getSuccessor(1);
		evar = man->SMT_mk_bool_var(getEdgeName(b,s));
		eexpr = man->SMT_mk_expr_from_bool_var(evar);
		components.push_back(bexpr);
		if (cond != NULL)
			components.push_back(cond);
		components_and = man->SMT_mk_and(components);
		rho_components.push_back(man->SMT_mk_eq(eexpr,components_and));
	}
}

void SMTpass::visitSwitchInst (SwitchInst &I) {
}

void SMTpass::visitIndirectBrInst (IndirectBrInst &I) {
}

void SMTpass::visitInvokeInst (InvokeInst &I) {
	BasicBlock * b = I.getParent();

	SMT_var cvar = man->SMT_mk_bool_var(getUndeterministicChoiceName(&I));
	SMT_expr cexpr = man->SMT_mk_expr_from_bool_var(cvar);

	SMT_var bvar = man->SMT_mk_bool_var(getNodeName(b,true));
	SMT_expr bexpr = man->SMT_mk_expr_from_bool_var(bvar);
	
	// normal destination
	BasicBlock * s = I.getNormalDest();
	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	SMT_expr eexpr = man->SMT_mk_expr_from_bool_var(evar);

	std::vector<SMT_expr> components;
	components.push_back(bexpr);
	components.push_back(cexpr);
	rho_components.push_back(man->SMT_mk_eq(eexpr,man->SMT_mk_and(components)));

	// Unwind destination
	s = I.getUnwindDest();
	evar = man->SMT_mk_bool_var(getEdgeName(b,s));
	eexpr = man->SMT_mk_expr_from_bool_var(evar);
	components.pop_back();
	components.push_back(man->SMT_mk_not(cexpr));
	rho_components.push_back(man->SMT_mk_eq(eexpr,man->SMT_mk_and(components)));
}

void SMTpass::visitUnwindInst (UnwindInst &I) {
}

void SMTpass::visitUnreachableInst (UnreachableInst &I) {
}

void SMTpass::visitICmpInst (ICmpInst &I) {
}

void SMTpass::visitFCmpInst (FCmpInst &I) {
}

void SMTpass::visitAllocaInst (AllocaInst &I) {
}

void SMTpass::visitLoadInst (LoadInst &I) {
}

void SMTpass::visitStoreInst (StoreInst &I) {
}

void SMTpass::visitGetElementPtrInst (GetElementPtrInst &I) {
}

SMT_expr SMTpass::construct_phi_ite(PHINode &I, unsigned i, unsigned n) {
	if (i == n-1) {
		// this is the last possible value of the PHI-variable
		return getValueExpr(I.getIncomingValue(i), primed[NULL]);
	}
	SMT_expr incomingVal = 	getValueExpr(I.getIncomingValue(i), primed[NULL]);

	SMT_var evar = man->SMT_mk_bool_var(getEdgeName(I.getIncomingBlock(i),I.getParent()));
	SMT_expr incomingBlock = man->SMT_mk_expr_from_bool_var(evar);

	SMT_expr tail = construct_phi_ite(I,i+1,n);
	return man->SMT_mk_ite(incomingBlock,incomingVal,tail);
}

void SMTpass::visitPHINode (PHINode &I) {
	ap_texpr_rtype_t ap_type;
	if (get_ap_type((Value*)&I, ap_type)) return;

	// we prime this PHI variable iff it is a Phivar from a block in Pr
	if (Pr[I.getParent()->getParent()]->count(I.getParent())) {
		primed[I.getParent()].insert(&I);
	}
	SMT_expr expr = getValueExpr(&I, primed[I.getParent()]);	
	SMT_expr assign = construct_phi_ite(I,0,I.getNumIncomingValues());

	if (!primed[I.getParent()].count(&I) && I.getNumIncomingValues() != 1) {
		SMT_var bvar = man->SMT_mk_bool_var(getNodeName(I.getParent(),false));
		SMT_expr bexpr = man->SMT_mk_not(man->SMT_mk_expr_from_bool_var(bvar));
		std::vector<SMT_expr> disj;
		disj.push_back(bexpr);
		disj.push_back(man->SMT_mk_eq(expr,assign));
		expr = man->SMT_mk_or(disj);	
	} else {
		expr = man->SMT_mk_eq(expr,assign);
	}
	rho_components.push_back(expr);
}

void SMTpass::visitTruncInst (TruncInst &I) {
}

void SMTpass::visitZExtInst (ZExtInst &I) {
}

void SMTpass::visitSExtInst (SExtInst &I) {
}

void SMTpass::visitFPTruncInst (FPTruncInst &I) {
}

void SMTpass::visitFPExtInst (FPExtInst &I) {
}

void SMTpass::visitFPToUIInst (FPToUIInst &I) {
}

void SMTpass::visitFPToSIInst (FPToSIInst &I) {
}

void SMTpass::visitUIToFPInst (UIToFPInst &I) {
}

void SMTpass::visitSIToFPInst (SIToFPInst &I) {
}

void SMTpass::visitPtrToIntInst (PtrToIntInst &I) {
}

void SMTpass::visitIntToPtrInst (IntToPtrInst &I) {
}

void SMTpass::visitBitCastInst (BitCastInst &I) {
}

void SMTpass::visitSelectInst (SelectInst &I) {
}

void SMTpass::visitCallInst(CallInst &I) {
}

void SMTpass::visitVAArgInst (VAArgInst &I) {
}

void SMTpass::visitExtractElementInst (ExtractElementInst &I) {
}

void SMTpass::visitInsertElementInst (InsertElementInst &I) {
}

void SMTpass::visitShuffleVectorInst (ShuffleVectorInst &I) {
}

void SMTpass::visitExtractValueInst (ExtractValueInst &I) {
}

void SMTpass::visitInsertValueInst (InsertValueInst &I) {
}

void SMTpass::visitTerminatorInst (TerminatorInst &I) {
}

void SMTpass::visitBinaryOperator (BinaryOperator &I) {
	ap_texpr_rtype_t ap_type;
	int t = get_ap_type((Value*)&I, ap_type);

	//primed[I.getParent()].insert(&I);
	//exist_prime.insert(&I);
	SMT_expr expr = getValueExpr(&I, primed[I.getParent()]);	
	SMT_expr assign = NULL;	
	std::vector<SMT_expr> operands;
	operands.push_back(getValueExpr(I.getOperand(0), primed[NULL]));
	operands.push_back(getValueExpr(I.getOperand(1), primed[NULL]));
	switch(I.getOpcode()) {
		// Standard binary operators...
		case Instruction::Add : 
		case Instruction::FAdd: 
			assign = man->SMT_mk_sum(operands);
			break;
		case Instruction::Sub : 
		case Instruction::FSub: 
			assign = man->SMT_mk_sub(operands);
			break;
		case Instruction::Mul : 
		case Instruction::FMul: 
			assign = man->SMT_mk_mul(operands);
			break;
		case Instruction::And :
			if (!t) return;
			assign = man->SMT_mk_and(operands);
			break;
		case Instruction::Or  :
			if (!t) return;
			assign = man->SMT_mk_or(operands);
			break;
		case Instruction::Xor :
			if (!t) return;
			assign = man->SMT_mk_xor(operands[0],operands[1]);
			break;
		case Instruction::UDiv: 
		case Instruction::SDiv: 
		case Instruction::FDiv: 
			assign = man->SMT_mk_div(operands[0],operands[1]);
			break;
		case Instruction::URem: 
		case Instruction::SRem: 
		case Instruction::FRem: 
			assign = man->SMT_mk_rem(operands[0],operands[1]);
			break;
			// the others are not implemented
		case Instruction::Shl : // Shift left  (logical)
		case Instruction::LShr: // Shift right (logical)
		case Instruction::AShr: // Shift right (arithmetic)
		case Instruction::BinaryOpsEnd:
			// NOT IMPLEMENTED
			return;
	}
	//instructions.push_back(man->SMT_mk_eq(expr,assign));
	rho_components.push_back(man->SMT_mk_eq(expr,assign));
}

void SMTpass::visitCmpInst (CmpInst &I) {
}

void SMTpass::visitCastInst (CastInst &I) {
}
