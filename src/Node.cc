#include<stack>
#include<map>

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
