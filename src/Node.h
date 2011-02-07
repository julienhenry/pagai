#ifndef _NODE_H
#define _NODE_H

#include<stack>

#include "llvm/BasicBlock.h"


using namespace llvm;

class node {
private:
	BasicBlock * bb;
	int sccId;
	bool loop;

	/* used by computeSCC */
	int color;
	int index;
	int lowlink;
	bool isInStack;
	void computeSCC_rec(int n,std::stack<node*> * S);

public:
	node(BasicBlock * _bb) : bb(_bb), loop(false), color(0), index(0), lowlink(0), isInStack(false) {}

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

#endif
