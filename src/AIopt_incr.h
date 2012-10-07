/**
 * \file AIopt_incr.h
 * \brief Declaration of the AIopt_incr class
 * \author Julien Henry
 */
#ifndef _AIOPT_INCR_H
#define _AIOPT_INCR_H

#include <queue>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"

#include "AIpass.h"

using namespace llvm;

/**
 * \class AIopt_incr
 * \brief AIopt Implementation that uses the result of a previous analysis
 */
class AIopt_incr : public ModulePass, public AIPass {

	private:
		/**
		 * \brief remembers all the paths that have already been
		 * visited
		 */
		std::map<BasicBlock*,PathTree*> pathtree;

		PathTree * W;

		std::map<BasicBlock*,PathTree*> U;
		std::map<BasicBlock*,PathTree*> V;

		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A_prime;

		void computeNewPaths(
			Node * n
			);

		void init()
			{
				aman = new AbstractManClassic();
				passID.T = COMBINED_INCR;
			}

	public:
		static char ID;	

	public:

		AIopt_incr(char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : ModulePass(_ID), AIPass(_man,_NewNarrow, _Threshold) {
			init();
			passID.D = _man;
			passID.N = _NewNarrow;
			passID.TH = _Threshold;
		}
		
		AIopt_incr() : ModulePass(ID) {
			init();
			passID.D = getApronManager();
			passID.N = useNewNarrowing();
			passID.TH = useThreshold();
		}


		~AIopt_incr () {
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

		/**
		 * \brief compute and update the Abstract value of the Node n
		 * \param n the starting point
		 */
		void computeNode(Node * n);
		
		/**
		 * \brief apply narrowing at node n
		 * \param n the starting point
		 */
		void narrowNode(Node * n);
};

#endif
