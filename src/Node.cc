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
	X = new Abstract(_man,env);
	Y = new Abstract(_man,env);
	widening = 0;
}

Node::~Node() {
	delete X;
	delete Y;
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

	if (get_ap_type(val,type)) return;

	switch (type) {
		case AP_RTYPE_INT:
			intVar[var].insert(val);
			env = ap_environment_alloc(&var,1,NULL,0);
			break;
		default:
			realVar[var].insert(val);
			env = ap_environment_alloc(NULL,0,&var,1);
			break;
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	set_ap_expr(val,exp);
}

void Node::create_env(ap_environment_t ** env) {

	ap_var_t * intvars = (ap_var_t*)malloc(intVar.size()*sizeof(ap_var_t));
	ap_var_t * realvars = (ap_var_t*)malloc(realVar.size()*sizeof(ap_var_t));

	int j = 0;
	for (std::map<ap_var_t,std::set<Value*> >::iterator i = intVar.begin(),
			e = intVar.end(); i != e; ++i) {
		intvars[j] = (*i).first;
		j++;
	}

	j = 0;
	for (std::map<ap_var_t,std::set<Value*> >::iterator i = realVar.begin(), 
			e = realVar.end(); i != e; ++i) {
		realvars[j] = (*i).first;
		j++;
	}

	if (*env != NULL)
		ap_environment_free(*env);
	*env = ap_environment_alloc(intvars,intVar.size(),realvars,realVar.size());
	
	free(intvars);
	free(realvars);
}

bool NodeCompare::operator() (Node * n1, Node * n2) {
	if (n1->sccId < n2->sccId) return true;
	return (n1->id > n2->id);
}
