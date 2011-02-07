#include<stack>

#include "llvm/BasicBlock.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "Node.h"
#include "Hashtables.h"

using namespace llvm;

/**
 * compute the strongly connected components and the loop heads of the graph.
 */
void node::computeSCC() {
	std::stack<node*> * S = new std::stack<node*>();
	computeSCC_rec(1,S);
}

/**
 * recursive version of the trojan's algorithm
 * compute both the loop heads and the Strongly connected components
 * Must be called with n=1 and and empty allocated stack
 */
void node::computeSCC_rec(int n,std::stack<node*> * S) {
	node * nsucc;

	index=n;
	lowlink=n;
	color=1;
	S->push(this);
	isInStack=true;
	for (succ_iterator s = succ_begin(bb), E = succ_end(bb); s != E; ++s) {
		BasicBlock * succ = *s;
		nsucc = nodes[succ];
		switch (nsucc->getIndex()) {
			case 0:
				nsucc->computeSCC_rec(n+1,S);
				lowlink = std::min(lowlink,nsucc->getLowlink());
			default:
				if (nsucc->getColor() == 1) {
					nsucc->setLoop(true);
				}
				if (nsucc->inStack()) {
					lowlink = std::min(lowlink,nsucc->getIndex());
				}
		}
	}
	color=2;
	if (lowlink == index) {
		do {
			nsucc = S->top();
			S->pop();
			nsucc->isInStack=false;
			nsucc->sccId = index;
		} while (nsucc != this);
	}
}

bool node::inStack() {
	return isInStack;
}

int node::getSccId() {
	return sccId;
}


int node::getColor() {
	return color;
}

void node::setColor(int n) {
	color = n;
}

int node::getIndex() {
	return index;
}

void node::setIndex(int n) {
	index = n;
}

int node::getLowlink() {
	return lowlink;
}

void node::setLowlink(int n) {
	lowlink = n;
}

int node::getLoop() {
	return loop;
}

void node::setLoop(bool b) {
	loop = b;
}
