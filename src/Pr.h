#ifndef PR_H
#define PR_H

#include <map>
#include <set>

#include "llvm/Support/CFG.h"
#include "Node.h"

using namespace llvm;

/// @brief Pr computation pass


class Pr {

	private:
		Function * F;

		/// associate to each formula its set of Pr nodes
		std::set<BasicBlock*> Pr_set;

		/// widening points (subset of Pr where we apply widening)
		std::set<BasicBlock*> Pw_set;

		std::set<BasicBlock*> Assert_set;
		std::set<BasicBlock*> UndefBehaviour_set;

		std::map<Node*,int> index;
		std::map<Node*,int> lowlink;
		std::map<Node*,bool> isInStack;

		/// compute the set Pr for a function
		void computePr();

		bool check_acyclic(std::set<BasicBlock*>* FPr);
		bool check_acyclic_rec(
				Node * n, 
				int & N,
				std::stack<Node*> * S,
				std::set<BasicBlock*>* FPr);

		bool computeLoopHeaders(std::set<BasicBlock*>* FPr);
		bool computeLoopHeaders_rec(
				Node * n, 
				std::set<Node*> * Seen,
				std::set<Node*> * S,
				std::set<BasicBlock*>* FPr);

		void minimize_Pr();

		// private constructor
		Pr(Function * F);

	public:
		static char ID;	

		static Pr * getInstance(Function * F);

		static void releaseMemory();

		~Pr();

		/// associate to each basicBlock its successors in Pr
		/// WARNING : this set is filled by SMTpass
		std::map<BasicBlock*,std::set<BasicBlock*> > Pr_succ;
		/// associate to each basicBlock its predecessors in Pr
		/// WARNING : this set is filled by SMTpass
		std::map<BasicBlock*,std::set<BasicBlock*> > Pr_pred;

		/// getPr - get the set Pr. The set Pr is computed only once
		std::set<BasicBlock*>* getPr();

		/// getPw - get the set Pw. The set Pw is computed only once
		std::set<BasicBlock*>* getPw();

		std::set<BasicBlock*>* getAssert();
		std::set<BasicBlock*>* getUndefinedBehaviour();

		bool inPr(BasicBlock * b);
		bool inPw(BasicBlock * b);
		bool inAssert(BasicBlock * b);
		bool inUndefBehaviour(BasicBlock * b);

		/// getPrPredecessors - returns a set containing all the predecessors of
		/// b in Pr
		std::set<BasicBlock*> getPrPredecessors(BasicBlock * b);

		/// getPrPredecessors - returns a set containing all the successors of
		/// b in Pr
		std::set<BasicBlock*> getPrSuccessors(BasicBlock * b);
};

extern std::map<Function *, Pr *> PR_instances;

#endif
