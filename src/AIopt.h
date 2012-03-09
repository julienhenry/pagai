#ifndef _AIOPT_H
#define _AIOPT_H

#include <queue>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"

#include "AIpass.h"

using namespace llvm;

/// Path Focusing implementation with Lookahead Widening
class AIopt : public ModulePass, public AIPass {

	private:
		/// paths - remembers all the paths that have already been
		/// visited
		std::map<BasicBlock*,PathTree*> pathtree;

		PathTree * W;

		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A_prime;

		void computeNewPaths(
			Node * n
			);
	public:
		static char ID;	

	public:

		AIopt(char &_ID, Apron_Manager_Type _man, bool _NewNarrow, bool _Threshold) : ModulePass(_ID), AIPass(_man,_NewNarrow, _Threshold) {
			init();
			passID.D = _man;
			passID.N = _NewNarrow;
			passID.TH = _Threshold;
		}
		
		AIopt() : ModulePass(ID) {
			init();
			passID.D = getApronManager();
			passID.N = useNewNarrowing();
			passID.TH = useThreshold();
		}

		void init()
			{
				//aman = new AbstractManGopan();
				aman = new AbstractManClassic();
				passID.T = LW_WITH_PF;
				//Passes[LW_WITH_PF] = passID;	
			}

		~AIopt () {
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
};

#endif
