#ifndef _NODE_H
#define _NODE_H

#include<stack>
#include<map>
#include<set>
#include<vector>

#include "llvm/BasicBlock.h"

#include "ap_global1.h"

#include "Abstract.h"

using namespace llvm;

typedef struct _phivar {
	std::vector<ap_var_t> name;
	std::vector<ap_texpr1_t> expr;
} phivar;

class Node {
	private:
		/// used by computeSCC
		int index;
		int lowlink;
		bool isInStack;
		ap_manager_t * man;
		void computeSCC_rec(int & n,std::stack<Node*> * S);
	public:
		/// bb - Basicblock associated to the node
		BasicBlock * bb;
		/// sccId identifies the strongly connected component the node is in
		int sccId;
		int id;
		/// X - Abstract domain 
		Abstract * X;
		ap_environment_t * env;

		/// intVar - contains all the int variables that have to be used as
		/// dimensions for the abstract value. Each variable is associated to a
		/// list of Values, that directly use this variable : if one of these
		/// Value is live, then the variable should not be removed from the Abstract
		/// domain's dimensions
		std::map<ap_var_t,std::set<Value*> > intVar;
		
		/// realVar - same as intVar, but for real variables
		std::map<ap_var_t,std::set<Value*> > realVar;

		/// tcons - contains the constraints for the outgoing transitions 
		std::map<Node*, std::vector<ap_tcons1_array_t*> > tcons;
		
		/// phi_vars - associates the right expression to each PHI-variable,
		/// depending on the incoming block
		std::map<Node*,phivar> phi_vars;

	public:
		Node(ap_manager_t * man, BasicBlock * _bb);
		~Node();

		/// computeSCC - compute the strongly connected components of the CFG 
		void computeSCC();
		
		/// add_var - add a new variable into the abstract domain
		void add_var(Value * val);
		
		/// create_env - creates an environment containing all live variables
		/// that needs to be used as dimension in the abstract domain
		void create_env(ap_environment_t ** env);

};

/// Nodes - Map that associate each BasicBlock of the Module with its Node
/// object
extern std::map<BasicBlock *,Node *> Nodes;

/// NodeCompare - This class is used to order the Nodes such that they are
/// poped in the right order when treated by the AI algorithm. 
class NodeCompare {
	public:
		bool operator() (Node * n1, Node * n2);
};
#endif
