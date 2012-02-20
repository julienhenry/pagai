#ifndef _AIDIS_H
#define _AIDIS_H

#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"

#include "AIpass.h"
#include "Sigma.h"

using namespace llvm;

/// Pass that computes disjunctive invariants
class AIdis : public ModulePass, public AIPass {

	private:
		std::map<BasicBlock*,Sigma*> S;

		int Max_Disj;

		/// paths - remembers all the paths that have already been
		/// visited
		std::map<BasicBlock*,PathTree*> pathtree;

		std::set<Node*> A_prime;

		void computeNewPaths(
			Node * n
			);

		int sigma(
			std::list<BasicBlock*> path, 
			int start,
			Abstract * Xtemp,
			bool source);

	public:
		static char ID;	

	public:

		AIdis(char &_ID, Apron_Manager_Type _man, bool _NewNarrow) : ModulePass(_ID), AIPass(_man,_NewNarrow) {
			init();
			passID.D = _man;
		}
		
		AIdis() : ModulePass(ID) {
			init();
			passID.D = getApronManager();
		}
		
		void init()
			{
				Max_Disj = 5;
				aman = new AbstractManDisj();
				passID.T = LW_WITH_PF_DISJ;
			}

		~AIdis () {
			for (std::map<BasicBlock*,PathTree*>::iterator 
				it = pathtree.begin(),
				et = pathtree.end(); 
				it != et; 
				it++) {
				if ((*it).second != NULL)
					delete (*it).second;
				}
			}

		const char *getPassName() const;

		void getAnalysisUsage(AnalysisUsage &AU) const;

		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		std::set<BasicBlock*> getPredecessors(BasicBlock * b) const;
		std::set<BasicBlock*> getSuccessors(BasicBlock * b) const;

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);
		
		/// narrowNode - apply narrowing at node n
		void narrowNode(Node * n);

		void loopiter(
			Node * n, 
			int index,
			int Sigma,
			Abstract * &Xtemp,
			std::list<BasicBlock*> * path,
			bool &only_join,
			PathTree * const U);
};

#endif
