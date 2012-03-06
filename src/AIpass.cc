#include <vector>
#include <list>

#include "llvm/Instructions.h"
#include "llvm/Support/CFG.h"

#include "AIpass.h"
#include "Pr.h"
#include "AISimple.h"
#include "Expr.h"
#include "Live.h"
#include "Node.h"
#include "Debug.h"

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
			ignoreFunction.insert(F);
			while (!A.empty()) A.pop();
			return;
		}
	}
}

void AIPass::narrowingIter(Node * n) {
	A.push(n);
	is_computed.clear();
	while (!A.empty()) {
		Node * current = A.top();
		A.pop();
		narrowNode(current);
		if (unknown) {
			ignoreFunction.insert(n->bb->getParent());
			while (!A.empty()) A.pop();
			return;
		}
	}
}

void AIPass::initFunction(Function * F) {
	Node * n;
	CurrentAIpass = this;

	// we create the Node objects associated to each basicblock
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		//resetting parameters
		n = Nodes[i];
		n->intVar.clear();
		n->realVar.clear();
		n->Exprs.clear();
		n->env = ap_environment_alloc_empty();
		// creating an X_s and an X_d abstract value for this node
		if (LSMT == NULL
				||dynamic_cast<AISimple*>(this)
				|| Pr::getPr(*F)->count(i)) {
			n->X_s[passID] = aman->NewAbstract(man,n->env);
			n->X_d[passID] = aman->NewAbstract(man,n->env);
			n->X_i[passID] = aman->NewAbstract(man,n->env);
			n->X_f[passID] = aman->NewAbstract(man,n->env);
		} else {
			n->X_s[passID] = NULL;
			n->X_d[passID] = NULL;
			n->X_i[passID] = NULL;
			n->X_f[passID] = NULL;
		}
	}

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
		printBasicBlock(i);
	//*Out << *F;
}

void AIPass::printResult(Function * F) {
	BasicBlock * b;
	Node * n;
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		n = Nodes[b];
		if (Pr::getPr(*b->getParent())->count(b) && ignoreFunction.count(F) == 0) {
			Out->changeColor(raw_ostream::MAGENTA,true);
			*Out << "\n\nRESULT FOR BASICBLOCK: -------------------" << *b << "-----\n";
			Out->resetColor();
			//n->X_i[passID]->print(true);
			n->X_s[passID]->print(true);
			if (Pr::getAssert(*b->getParent())->count(b)) {
				if (n->X_s[passID]->is_bottom()) {
					Out->changeColor(raw_ostream::GREEN,true);
					*Out << "ASSERT OK\n";
				} else {
					Out->changeColor(raw_ostream::RED,true);
					*Out << "ASSERT FAILED\n";
				}
				Out->resetColor();
			}
		}
	}
	*Out << Total_time[passID][F]->seconds() << "." << Total_time[passID][F]->microseconds() << " seconds\n";
	//*Out << SMT_time.tv_sec << " " << SMT_time.tv_usec  << " SMT_TIME " << "\n";
	*Out << "ASC ITERATIONS " << asc_iterations[passID][F] << "\n" ;
	*Out << "DESC ITERATIONS " << desc_iterations[passID][F] << "\n" ;
}

void AIPass::printBasicBlock(BasicBlock* b) {
	Node * n = Nodes[b];
	//	*Out << b << ": SCC=" << n->sccId << ": LOOP HEAD" << *b;
	//} else {
	*Out << "\n(SCC=" << n->sccId << ")" << *b;
	//}
	//for (BasicBlock::iterator i = b->begin(), e = b->end(); i != e; ++i) {
	//	Instruction * I = i;
	//	*Out << "\t\t" << I << "\t" << *I << "\n";
	//}

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

bool AIPass::copy_Xd_to_Xs(Function * F) {
	BasicBlock * b;
	ap_environment_t * env = ap_environment_alloc_empty();
	bool res = false;

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		if (dynamic_cast<AISimple*>(this)
				|| Pr::getPr(*F)->count(i)) {

			if (!res && Nodes[b]->X_s[passID]->compare(Nodes[b]->X_d[passID]) != 0)
				res = true;

			delete Nodes[b]->X_s[passID];
			if (b != F->begin()) {
				Nodes[b]->X_s[passID] = Nodes[b]->X_d[passID];
				Nodes[b]->X_d[passID] = aman->NewAbstract(man,env);
			} else {
				Nodes[b]->X_s[passID] = aman->NewAbstract(Nodes[b]->X_d[passID]);
			}
		}
	}
	return res;
}

void AIPass::copy_Xs_to_Xf(Function * F) {
	BasicBlock * b;
	ap_environment_t * env = ap_environment_alloc_empty();

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		if (dynamic_cast<AISimple*>(this)
				|| Pr::getPr(*F)->count(i)) {

			delete Nodes[b]->X_f[passID];
			Nodes[b]->X_f[passID] = aman->NewAbstract(Nodes[b]->X_s[passID]);
		}
	}
}

void AIPass::copy_Xf_to_Xs(Function * F) {
	BasicBlock * b;
	ap_environment_t * env = ap_environment_alloc_empty();

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		b = i;
		if (dynamic_cast<AISimple*>(this)
				|| Pr::getPr(*F)->count(i)) {

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
			Xtemp->join_array(Xtemp->main->env,Join);

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
				Xtemp->widening_threshold(Succ->X_s[passID],&threshold);
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
			computeTransform(aman,n,*path,*Xtemp);
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
		ap_environment_t * env) {
	A->set_top(env);
	A->meet_tcons_array(C);
	ap_lincons1_array_t cs = A->to_lincons_array();
	if (ap_lincons1_array_size(&cs) > 0) {
		ap_lincons1_t c = ap_lincons1_array_get(&cs,0);
		ap_environment_t * e = ap_lincons1_envref(&c);
		if (ap_environment_is_leq(e,env)) {
			ap_lincons1_t tmp = ap_lincons1_copy(&c);
			cons->push_back(tmp);
		}
	}
	ap_lincons1_array_clear(&cs);
}

void AIPass::computeTransform (AbstractMan * aman, Node * n, std::list<BasicBlock*> path, Abstract &Xtemp) {

	// setting the focus path, such that the instructions can be correctly
	// handled
	focuspath.clear();
	constraints.clear();
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

	ap_environment_t * env = NULL;
	succ->create_env(&env,LV);

	// tmpenv contains all the environment of the starting invariant, plus the
	// new environment variables that have been added along the path
	ap_environment_t * tmpenv = common_environment(env,Xtemp.main->env);

	Xtemp.change_environment(tmpenv);

	// first, we assign the Phi variables defined during the path to the right expressions
	Xtemp.assign_texpr_array(&PHIvars.name[0],&PHIvars.expr[0],PHIvars.name.size(),NULL);

	// We create an Abstract Value that will represent the set of constraints
	Abstract * ConstraintsAbstract = aman->NewAbstract(man, env);

	//////////
	//

#if 1
	ap_var_t var;
	Value * val;
	std::vector<ap_var_t> intdims;
	std::vector<ap_var_t> realdims;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		val = (Value*)var;
		if (LV->isLiveByLinearityInBlock(val,succ->bb,true)) {
			intdims.push_back(var);
		}
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		val = (Value*)var;
		if (LV->isLiveByLinearityInBlock(val,succ->bb,true)) {
			realdims.push_back(var);
		}
	}
	ap_environment_t * env2 = ap_environment_alloc(&intdims[0], intdims.size(), &realdims[0], realdims.size());
#else
	ap_environment_t * env2 = env;
#endif
	/////////

	std::vector<ap_lincons1_t> cons;

	std::list<std::vector<ap_tcons1_array_t*>*>::iterator i, e;
	for (i = constraints.begin(), e = constraints.end(); i!=e; ++i) {
		if ((*i)->size() == 1) {
			DEBUG(
					tcons1_array_print((*i)->front());
				 );
			Xtemp.meet_tcons_array((*i)->front());

			computeThreshold((*i)->front(),&cons,ConstraintsAbstract,env2);

			ap_tcons1_array_clear((*i)->front());
		} else {
			DEBUG(
					*Out << "multiple contraints:\n";
				 );
			std::vector<Abstract*> A;
			// A_Constraints is used by ConstraintsAbstract
			std::vector<Abstract*> A_Constraints;
			std::vector<ap_tcons1_array_t*>::iterator it, et;
			Abstract * X2;
			for (it = (*i)->begin(), et = (*i)->end(); it != et; ++it) {
				DEBUG(
						tcons1_array_print(*it);
					 );
				X2 = aman->NewAbstract(&Xtemp);
				X2->meet_tcons_array(*it);
				A.push_back(X2);

				computeThreshold(*it,&cons,ConstraintsAbstract,env2);

				ap_tcons1_array_clear(*it);
			}
			Xtemp.join_array(env,A);
		}
		delete *i;
	}

	Xtemp.assign_texpr_array(&PHIvars_prime.name[0],&PHIvars_prime.expr[0],PHIvars_prime.name.size(),NULL);

	DEBUG(
			for (unsigned i = 0; i < PHIvars_prime.name.size(); i++) {
			ap_var_t name = PHIvars_prime.name[i];
			ap_texpr1_t expr = PHIvars_prime.expr[i];
			*Out << "Assigning " << ap_var_to_string(name) << " = ";
			texpr1_print(&expr);
			*Out << "\n";
			}
		 );

	/////////
	succ->env = env2;
	Xtemp.change_environment(env2);
	env = env2;
	////////

	// we transform the abstract value of constraints into an array of lincons
	// for a future widening with threshold
	ap_lincons1_array_clear(&threshold);
	threshold = ap_lincons1_array_make(env,cons.size());
	for (unsigned k = 0; k < cons.size(); k++) {
		if (!ap_lincons1_extend_environment_with(&cons[k],env))
			ap_lincons1_array_set(&threshold,k,&cons[k]);
	}
	delete ConstraintsAbstract;
}

// TODO :
// make it work for path-focused techniques
bool AIPass::computeWideningSeed(Function * F) {
	Abstract * Xtemp;
	Node * n;
	Node * Succ;
	std::list<BasicBlock*> path;
	bool found = false;

	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		n = Nodes[i];
		std::set<BasicBlock*> Succs = getSuccessors(n->bb);
		for (std::set<BasicBlock*>::iterator s = Succs.begin(), E = Succs.end(); s != E; ++s) {

			// to be candidate, the transition should go to a widening point
			if (!Pr::inPw(*s)) continue;

			path.clear();
			path.push_back(n->bb);
			path.push_back(*s);
			Succ = Nodes[*s];
			// computing the image of the abstract value by the path's tranformation
			Xtemp = aman->NewAbstract(n->X_s[passID]);
			computeTransform(aman,n,path,*Xtemp);

			// we check if the Abstract value is a good seed for Halbwachs's
			// narrowing
			if (Xtemp->compare(Succ->X_i[passID]) < 0) {

				Abstract * Xseed = aman->NewAbstract(Xtemp);
				std::vector<Abstract*> Join;
				Join.push_back(aman->NewAbstract(Xtemp));
				Join.push_back(aman->NewAbstract(Succ->X_i[passID]));
				Xseed->join_array(Xtemp->main->env,Join);
				if (Xseed->compare(Succ->X_s[passID]) == 1) {
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
					Succ->X_d[passID]->join_array(Xtemp->main->env,Join);
					A.push(Succ);
					found = true;
				}
			}
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

/// insert_env_vars_into_node_vars - this function takes all apron variables of
/// an environment, and adds them into the Node's variables, with a Value V as
/// a use.
void AIPass::insert_env_vars_into_node_vars(ap_environment_t * env, Node * n, Value * V) {
	ap_var_t var;
	for (size_t i = 0; i < env->intdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		n->intVar[V].insert(var);
	}
	for (size_t i = env->intdim; i < env->intdim + env->realdim; i++) {
		var = ap_environment_var_of_dim(env,i);
		n->realVar[V].insert(var);
	}
}

// create_constraints - this function is called by computeCondition
// it creates the constraint from its arguments and insert it into t_cons
void AIPass::create_constraints(
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


bool AIPass::computeCondition(	CmpInst * inst, 
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
		default:
			// unreachable
			constyp = AP_CONS_DISEQ; // disequality constraint
			nconstyp = AP_CONS_EQ;
			break;
	}
	if (result) {
		// creating the TRUE constraints
		create_constraints(constyp,expr,nexpr,cons);
	} else {
		// creating the FALSE constraints
		create_constraints(nconstyp,nexpr,expr,cons);
	}
	return true;
}

bool AIPass::computeConstantCondition(	ConstantInt * inst, 
		bool result,
		std::vector<ap_tcons1_array_t*> * cons) {

	// this is always true
	return false;

	//	if (result) {
	//		// always true
	//		return false;
	//	}

	//	// we create a unsat constraint
	//	// such as one of the successor is unreachable
	//	ap_tcons1_t tcons;
	//	ap_tcons1_array_t * consarray;
	//	ap_environment_t * env = ap_environment_alloc_empty();
	//	consarray = new ap_tcons1_array_t();
	//	tcons = ap_tcons1_make(
	//			AP_CONS_SUP,
	//			ap_texpr1_cst_scalar_double(env,1.),
	//			ap_scalar_alloc_set_double(0.));
	//	*consarray = ap_tcons1_array_make(tcons.env,1);
	//	ap_tcons1_array_set(consarray,0,&tcons);
	//	
	//	if (inst->isZero() && !result) {
	//		// condition is always false
	//		cons->push_back(consarray);
	//	}
}

bool AIPass::computePHINodeCondition(PHINode * inst, 
		bool result,
		std::vector<ap_tcons1_array_t*> * cons) {

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

			if (CmpInst * cmp = dyn_cast<CmpInst>(pv)) {
				ap_texpr_rtype_t ap_type;
				if (get_ap_type(cmp->getOperand(0), ap_type)) return false;
				res = computeCondition(cmp,result,cons);
			} else if (ConstantInt * c = dyn_cast<ConstantInt>(pv)) {
				res = computeConstantCondition(c,result,cons);
			} else if (PHINode * phi = dyn_cast<PHINode>(pv)) {
				// I.getOperand(0) could also be a
				// boolean PHI-variable
				res = computePHINodeCondition(phi,result,cons);
			} else {
				// loss of precision...
				res = false;
			}
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

	std::vector<ap_tcons1_array_t*> * cons = new std::vector<ap_tcons1_array_t*>();

	if (CmpInst * cmp = dyn_cast<CmpInst>(I.getOperand(0))) {
		ap_texpr_rtype_t ap_type;
		if (get_ap_type(cmp->getOperand(0), ap_type)) {
			delete cons;
			return;
		}
		res = computeCondition(cmp,test,cons);
	} else if (ConstantInt * c = dyn_cast<ConstantInt>(I.getOperand(0))) {
		res = computeConstantCondition(c,test,cons);
	} else if (PHINode * phi = dyn_cast<PHINode>(I.getOperand(0))) {
		// I.getOperand(0) could also be a
		// boolean PHI-variable
		res = computePHINodeCondition(phi,test,cons);
	} else {
		// loss of precision...
		delete cons;
		return;
	}

	// we add cons in the set of constraints of the path
	if (res) 
		constraints.push_back(cons);
	else
		delete cons;
}

/// for the moment, we only create the constraint for the default branch of the
/// switch. The other branches lose information...
/// This code is dead since we use the LowerSwitch pass before doing abstract
/// interpretation.
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

void AIPass::visitUnwindInst (UnwindInst &I){
	//*Out << "UnwindInst\n" << I << "\n";	
	visitInstAndAddVarIfNecessary(I);
}

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
	if (get_ap_type((Value*)&I, ap_type)) return;
	DEBUG(
			*Out << I << "\n";
		 );

	// if the PHINode has actually one single incoming edge, we just say the
	// value is equal to its associated expression
	// There is no need to introduce PHIvars...
	if (I.getNumIncomingValues() == 1) {
		pv = I.getIncomingValue(0);
		ap_texpr1_t * expr = get_ap_expr(n,pv);
		set_ap_expr(&I,expr,n);
		ap_environment_t * env = expr->env;
		insert_env_vars_into_node_vars(env,n,(Value*)&I);
		return;
	}

	for (unsigned i = 0; i < I.getNumIncomingValues(); i++) {
		if (pred == I.getIncomingBlock(i)) {
			pv = I.getIncomingValue(i);
			nb = Nodes[I.getIncomingBlock(i)];
			ap_texpr1_t * expr = get_ap_expr(n,pv);

			if (focusblock == focuspath.size()-1) {
				if (LV->isLiveByLinearityInBlock(&I,n->bb,true)) {
					n->add_var(&I);
					PHIvars_prime.name.push_back((ap_var_t)&I);
					PHIvars_prime.expr.push_back(*expr);
					ap_environment_t * env = expr->env;
					insert_env_vars_into_node_vars(env,n,(Value*)&I);
					DEBUG(
							*Out << I << " is equal to ";
							texpr1_print(expr);
							*Out << "\n";
						 );
				}
			} else {
				if (expr == NULL) continue;
				DEBUG(
						*Out << I << " is equal to ";
						texpr1_print(expr);
						*Out << "\n";
					 );
				if (LV->isLiveByLinearityInBlock(&I,n->bb,true)) {
					n->add_var(&I);
					PHIvars.name.push_back((ap_var_t)&I);
					PHIvars.expr.push_back(*expr);
				} else {
					set_ap_expr(&I,expr,n);
				}
				ap_environment_t * env = expr->env;
				insert_env_vars_into_node_vars(env,n,(Value*)&I);
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
			// we consider the result is unknown
			// so we create a new variable
			visitInstAndAddVarIfNecessary(I);
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
	set_ap_expr(&I,exp,n);

	// this value may use some apron variables 
	// we add these variables in the Node's variable structure, such that we
	// remember that instruction I uses these variables
	//
	ap_environment_t * env = exp->env;
	insert_env_vars_into_node_vars(env,n,(Value*)&I);
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
	ap_environment_t* env = NULL;
	ap_var_t var = (Value *) &I; 

	ap_texpr_rtype_t ap_type;

	if (get_ap_type((Value*)&I, ap_type)) return;

	if (!LV->isLiveByLinearityInBlock(&I,I.getParent(),false))
		return;

	if (ap_type == AP_RTYPE_INT) { 
		env = ap_environment_alloc(&var,1,NULL,0);
	} else {
		env = ap_environment_alloc(NULL,0,&var,1);
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	set_ap_expr(&I,exp,n);
	if (LV->isLiveByLinearityInBlock(&I,n->bb,true))
		n->add_var((Value*)var);
}


