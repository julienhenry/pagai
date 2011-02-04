#ifndef _NODE_H
#define _NODE_H

#include "llvm/BasicBlock.h"

using namespace llvm;

class node {
private:
	BasicBlock * bb;
	int WTO;	
	bool Loop;
public:
	
	node(BasicBlock * _bb) : bb(_bb), WTO(0), Loop(false) {}

	void compute_loop_heads();

	int getWTO();
	void setWTO(int n);

	int getLoop();
	void setLoop(bool b);
};

#endif
