#ifndef _NODE_H
#define _NODE_H

#include<stack>
#include<map>

#include "llvm/BasicBlock.h"

#include "ap_global1.h"

using namespace llvm;


class Node {
private:
	/*Basicblock associated to the node*/
	BasicBlock * bb;

	/* used by computeSCC */
	int index;
	int lowlink;
	bool isInStack;
	void computeSCC_rec(int n,std::stack<Node*> * S);

	/*Abstract domains */
	ap_abstract1_t X;

public:
	/*identifies the strongly connected component the node is in*/
	int sccId;


public:
	Node(BasicBlock * _bb) : bb(_bb), index(0), lowlink(0), isInStack(false) {}

	/*compute the strongly connected components of the CFG*/
	void computeSCC();
};

extern std::map<BasicBlock *,Node *> Nodes;

#endif
