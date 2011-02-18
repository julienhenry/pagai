#include<stack>
#include<map>
#include<set>

#include "llvm/BasicBlock.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "Node.h"
#include "Expr.h"

using namespace llvm;

std::map<BasicBlock *,Node *> Nodes;


Node::~Node() {
	ap_manager_t* man = ap_abstract0_manager(X->abstract0);
	ap_abstract1_fprint(stdout,man,X);
	ap_abstract1_clear(man,X);
}

/**
 * compute the strongly connected components and the loop heads of the graph.
 */
void Node::computeSCC() {
	std::stack<Node*> * S = new std::stack<Node*>();
	computeSCC_rec(1,S);
	delete S;
}

/**
 * recursive version of the trojan's algorithm
 * compute both the loop heads and the Strongly connected components
 * Must be called with n=1 and and empty allocated stack
 */
void Node::computeSCC_rec(int n,std::stack<Node*> * S) {
	Node * nsucc;
	index=n;
	lowlink=n;
	S->push(this);
	isInStack=true;
	for (succ_iterator s = succ_begin(bb), E = succ_end(bb); s != E; ++s) {
		BasicBlock * succ = *s;
		nsucc = Nodes[succ];
		switch (nsucc->index) {
			case 0:
				nsucc->computeSCC_rec(n+1,S);
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
	switch (Expr::get_ap_type(val)) {
		case AP_RTYPE_INT:
			intVar.insert(var);
			env = ap_environment_alloc(&var,1,NULL,0);
			break;
		default:
			realVar.insert(var);
			env = ap_environment_alloc(NULL,0,&var,1);
			break;
	}
	ap_texpr1_t * exp = ap_texpr1_var(env,var);
	Expr::set_ap_expr(val,exp);
}

void Node::create_env(ap_environment_t ** env) {

	ap_var_t * intvars = (ap_var_t*)malloc(intVar.size()*sizeof(ap_var_t));
	ap_var_t * realvars = (ap_var_t*)malloc(realVar.size()*sizeof(ap_var_t));
	copy (intVar.begin(),intVar.end(),intvars);
	copy (realVar.begin(),realVar.end(),realvars);

	if (*env != NULL)
		ap_environment_free(*env);
	*env = ap_environment_alloc(intvars,intVar.size(),realvars,realVar.size());
	free(intvars);
	free(realvars);
}

bool NodeCompare::operator() (Node * n1, Node * n2) {
	return (n1->sccId > n2->sccId);
}
