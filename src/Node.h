#ifndef _NODE_H
#define _NODE_H

#include<stack>
#include<map>

#include "llvm/BasicBlock.h"

#include "ap_global1.h"

using namespace llvm;


class Node {
private:
	BasicBlock * bb;
	int sccId;
	bool loop;

	/* used by computeSCC */
	int color;
	int index;
	int lowlink;
	bool isInStack;
	void computeSCC_rec(int n,std::stack<Node*> * S);

	/*Abstract domains */
	ap_abstract1_t X_begin;
	ap_abstract1_t X_end;

public:
	Node(BasicBlock * _bb) : bb(_bb), loop(false), color(0), index(0), lowlink(0), isInStack(false) {}

	void computeSCC();

	int getSccId();

	int getColor();
	void setColor(int n);

	int getLowlink();
	void setLowlink(int n);

	int getIndex();
	void setIndex(int n);

	bool inStack();

	int getLoop();
	void setLoop(bool b);
};

extern std::map<BasicBlock *,Node *> Nodes;

#endif
