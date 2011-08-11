#include<stack>
#include<map>
#include<set>

#include "llvm/BasicBlock.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Node.h"
#include "Expr.h"
#include "Abstract.h"
#include "Debug.h"
#include "Analyzer.h"
#include "AbstractClassic.h"
#include "AbstractGopan.h"

using namespace llvm;

std::map<BasicBlock *,Node *> Nodes;

int i = 0;


Node::Node(ap_manager_t * _man, BasicBlock * _bb) {
	index = 0;
	lowlink = 0;
	isInStack = false;
	bb = _bb;
	id = i++;
	man = _man;
	env = ap_environment_alloc_empty();
	X = new AbstractClassic(_man,env);
	Y = new AbstractClassic(_man,env);
	//X = new AbstractGopan(_man,env);
	//Y = new AbstractGopan(_man,env);
	Xgopan = new AbstractGopan(_man,env);
	Ygopan = new AbstractGopan(_man,env);
}

Node::~Node() {
	delete X;
	delete Y;
	delete Xgopan;
	delete Ygopan;
}

/// computeSCC - compute the strongly connected components and the loop 
/// heads of the graph.
///
void Node::computeSCC() {
	std::stack<Node*> * S = new std::stack<Node*>();
	int n = 1;
	computeSCC_rec(n,S);
	delete S;
}

/// computeSCC_rec -  recursive version of the tarjan's algorithm
/// compute both the loop heads and the Strongly connected components
/// Must be called with n=1 and and empty allocated stack
///
void Node::computeSCC_rec(int & n,std::stack<Node*> * S) {
	Node * nsucc;
	index=n;
	lowlink=n;
	n++;
	S->push(this);
	isInStack=true;
	for (succ_iterator s = succ_begin(bb), E = succ_end(bb); s != E; ++s) {
		BasicBlock * succ = *s;
		nsucc = Nodes[succ];
		switch (nsucc->index) {
			case 0:
				nsucc->computeSCC_rec(n,S);
				lowlink = std::min(lowlink,nsucc->lowlink);
			default:
				if (nsucc->isInStack) {
					lowlink = std::min(lowlink,nsucc->index);
				}
		}
	}
	if (lowlink == index) {
		do {
			nsucc = S->top();
			S->pop();
			nsucc->isInStack=false;
			nsucc->sccId = index;
		} while (nsucc != this);
	}
}

void Node::add_var(Value * val) {
	ap_environment_t* env;
	ap_var_t var = val; 
	ap_texpr_rtype_t type;

	if (get_ap_type(val,type)) {
		return;
	}

	switch (type) {
		case AP_RTYPE_INT:
			intVar[val].insert(var);
			env = ap_environment_alloc(&var,1,NULL,0);
			break;
		default:
			realVar[val].insert(var);
			env = ap_environment_alloc(NULL,0,&var,1);
			break;
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	set_ap_expr(val,exp);
}

void Node::create_env(ap_environment_t ** env, Live * LV) {
	std::set<ap_var_t> Sintvars;
	std::set<ap_var_t> Srealvars;

	for (std::map<Value*,std::set<ap_var_t> >::iterator i = intVar.begin(),
			e = intVar.end(); i != e; ++i) {
		if (LV->isLiveThroughBlock((*i).first,bb)
			|| LV->isUsedInBlock((*i).first,bb)
			|| isa<UndefValue>((*i).first))
			Sintvars.insert((*i).second.begin(), (*i).second.end());
	}
	for (std::map<Value*,std::set<ap_var_t> >::iterator i = realVar.begin(),
			e = realVar.end(); i != e; ++i) {
		if (LV->isLiveThroughBlock((*i).first,bb)
			|| LV->isUsedInBlock((*i).first,bb)
			|| isa<UndefValue>((*i).first))
			Srealvars.insert((*i).second.begin(), (*i).second.end());
	}

	ap_var_t * intvars = (ap_var_t*)malloc(Sintvars.size()*sizeof(ap_var_t));
	ap_var_t * realvars = (ap_var_t*)malloc(Srealvars.size()*sizeof(ap_var_t));

	int j = 0;
	for (std::set<ap_var_t>::iterator i = Sintvars.begin(),
			e = Sintvars.end(); i != e; ++i) {
		intvars[j] = *i;
		j++;
	}

	j = 0;
	for (std::set<ap_var_t>::iterator i = Srealvars.begin(),
			e = Srealvars.end(); i != e; ++i) {
		realvars[j] = *i;
		j++;
	}

	if (*env != NULL)
		ap_environment_free(*env);
	*env = ap_environment_alloc(intvars,Sintvars.size(),realvars,Srealvars.size());
	
	free(intvars);
	free(realvars);
}

bool NodeCompare::operator() (Node * n1, Node * n2) {
	if (n1->sccId < n2->sccId) return true;
	return (n1->id > n2->id);
}
