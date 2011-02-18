#ifndef _NODE_H
#define _NODE_H

#include<stack>
#include<map>
#include<set>

#include "llvm/BasicBlock.h"

#include "ap_global1.h"

using namespace llvm;

typedef struct _phivar {
	std::vector<ap_var_t> name;
	std::vector<ap_texpr1_t> expr;
} phivar;

class Node {
	private:
		/* used by computeSCC */
		int index;
		int lowlink;
		bool isInStack;
		void computeSCC_rec(int n,std::stack<Node*> * S);
	public:
		/* Basicblock associated to the node */
		BasicBlock * bb;
		/* identifies the strongly connected component the node is in */
		int sccId;
		/* Abstract domain */
		ap_abstract1_t * X;

		/* vector of int and real variables */
		std::set<ap_var_t> intVar;
		std::set<ap_var_t> realVar;

		/* contains the constraints for the outgoing transitions */
		std::map<Node*,ap_tcons1_array_t*> tcons;
		std::map<Node*,phivar> phi_vars;

	public:
		Node(BasicBlock * _bb): index(0), 
								lowlink(0), 
								isInStack(false), 
								bb(_bb), 
								X(NULL) {}
		~Node();

		/* compute the strongly connected components of the CFG */
		void computeSCC();

		void add_var(Value * val);
		
		void create_env(ap_environment_t ** env);

};

extern std::map<BasicBlock *,Node *> Nodes;

class NodeCompare {
	public:
		bool operator() (Node * n1, Node * n2);
};
#endif
