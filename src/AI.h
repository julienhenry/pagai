#ifndef _AI_H
#define _AI_H

#include <queue>
#include <vector>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Analysis/LiveValues.h"
#include "llvm/Analysis/LoopInfo.h"

#include "ap_global1.h"
#include "pk.h"

#include "apron.h"
#include "Node.h"
#include "Live.h"

using namespace llvm;

class AI : public ModulePass, public InstVisitor<AI> {

	private:
		/// LV - result of the Live pass
		Live * LV;
		/// LI - result of the LoopInfo pass
		LoopInfo * LI;
		/// A - list of active Nodes, that still have to be computed
		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A;
		/// is_computed - remember the Nodes that don't need to be recomputed.
		/// This is used to remove duplicates in the A list.
		std::map<Node*,bool> is_computed;
	
		/// man - apron manager we use along the pass
		ap_manager_t* man;

	public:
		static char ID;	

		AI () : ModulePass(ID) {
				man = pk_manager_alloc(true);
				init_apron();
			}

		~AI () {
				ap_manager_free(man);
			}

		const char *getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);

		void computeFunction(Function * F);

		/// initFunction - initialize the function by creating the Node
		/// objects, and computing the strongly connected components.
		void initFunction(Function * F);
		
		/// printBasicBlock - print a basicBlock on standard output
		void printBasicBlock(BasicBlock * b);
	
		/// computeEnv - compute the new environment of Node n, based on 
		/// its intVar and RealVar maps
		void computeEnv(Node * n);

		/// computeHull - compute the abstract domain resulting from the union
		/// of the abstract of all predecessors of Node n
		/// and assign it to Xtemp
		void computeHull(ap_environment_t * env, 
				Node * n, 
				Abstract &Xtemp, 
				bool &update);

		/// computeNode - compute and update the Abstract value of the Node n
		void computeNode(Node * n);

		/// computeCondition - creates the constraint arrays resulting from a
		/// comparison instruction.
		void computeCondition(	CmpInst * inst, 
				ap_tcons1_array_t ** true_cons, 
				ap_tcons1_array_t ** false_cons);

		// Visit methods
		void visitReturnInst (ReturnInst &I);
		void visitBranchInst (BranchInst &I);
		void visitSwitchInst (SwitchInst &I);
		void visitIndirectBrInst (IndirectBrInst &I);
		void visitInvokeInst (InvokeInst &I);
		void visitUnwindInst (UnwindInst &I);
		void visitUnreachableInst (UnreachableInst &I);
		void visitICmpInst (ICmpInst &I);
		void visitFCmpInst (FCmpInst &I);
		void visitAllocaInst (AllocaInst &I);
		void visitLoadInst (LoadInst &I);
		void visitStoreInst (StoreInst &I);
		void visitGetElementPtrInst (GetElementPtrInst &I);
		void visitPHINode (PHINode &I);
		void visitTruncInst (TruncInst &I);
		void visitZExtInst (ZExtInst &I);
		void visitSExtInst (SExtInst &I);
		void visitFPTruncInst (FPTruncInst &I);
		void visitFPExtInst (FPExtInst &I);
		void visitFPToUIInst (FPToUIInst &I);
		void visitFPToSIInst (FPToSIInst &I);
		void visitUIToFPInst (UIToFPInst &I);
		void visitSIToFPInst (SIToFPInst &I);
		void visitPtrToIntInst (PtrToIntInst &I);
		void visitIntToPtrInst (IntToPtrInst &I);
		void visitBitCastInst (BitCastInst &I);
		void visitSelectInst (SelectInst &I);
		void visitCallInst(CallInst &I);
		void visitVAArgInst (VAArgInst &I);
		void visitExtractElementInst (ExtractElementInst &I);
		void visitInsertElementInst (InsertElementInst &I);
		void visitShuffleVectorInst (ShuffleVectorInst &I);
		void visitExtractValueInst (ExtractValueInst &I);
		void visitInsertValueInst (InsertValueInst &I);
		void visitTerminatorInst (TerminatorInst &I);
		void visitBinaryOperator (BinaryOperator &I);
		void visitCmpInst (CmpInst &I);
		void visitCastInst (CastInst &I);


		void visitInstruction(Instruction &I) {
			ferrs() << I.getOpcodeName();
			assert(0 && "Instruction not interpretable yet!");
		}

};

#endif
