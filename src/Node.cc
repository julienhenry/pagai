#include<stack>
#include<map>
#include<set>

#include "llvm/BasicBlock.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "Node.h"
#include "Expr.h"

using namespace llvm;

std::map<BasicBlock *,Node *> Nodes;

/**
 * compute the strongly connected components and the loop heads of the graph.
 */
void Node::computeSCC() {
	std::stack<Node*> * S = new std::stack<Node*>();
	computeSCC_rec(1,S);
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
	
	switch (Expr::get_ap_type(val)) {
		case AP_RTYPE_INT:
			intVar.insert((ap_var_t)val);
			break;
		default:
			realVar.insert((ap_var_t)val);
			break;
	}
}

ap_environment_t * Node::create_env() {
	ap_var_t * intvars = new ap_var_t [intVar.size()];
	ap_var_t * realvars = new ap_var_t [realVar.size()];
	copy (intVar.begin(),intVar.end(),intvars);
	copy (realVar.begin(),realVar.end(),realvars);
	return ap_environment_alloc(intvars,intVar.size(),
								realvars,realVar.size());

}

bool NodeCompare::operator() (Node * n1, Node * n2) {
	return (n1->sccId < n2->sccId);
}
