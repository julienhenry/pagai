#include "llvm/BasicBlock.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "node.h"
#include "hashtables.h"

using namespace llvm;



void node::compute_loop_heads() {
	node * n;
	WTO=1;
	for (succ_iterator s = succ_begin(bb), E = succ_end(bb); s != E; ++s) {
		BasicBlock * succ = *s;
		n = nodes[succ];
		switch (n->getWTO()) {
			case 1:
				// n node is a head of loop
				fouts() << "head of loop detected : " << n->bb << "\n";	
				n->setLoop(true);
				break;
			case 0:
				n->compute_loop_heads();
				break;
			default:
				break;
		}
	}
	WTO=2;
}


int node::getWTO() {
	return WTO;
}

void node::setWTO(int n) {
	WTO = n;
}

int node::getLoop() {
	return Loop;
}

void node::setLoop(bool b) {
	Loop = b;
}
