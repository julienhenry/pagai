#ifndef _AIPASS_H
#define _AIPASS_H

#include <queue>
#include <vector>

#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Constants.h"

#include "Analyzer.h"
#include "apron.h"
#include "PathTree.h"
#include "AbstractMan.h"

using namespace llvm;

class SMTpass;
class Live;
class Node;

/// @brief Base class for abstract interpretation
///
/// Base class factoring helper functions and data-structure to
/// perform Abstract Interpretation (i.e. graph traversal on the CFG,
/// Apron Manager, SMTpass solver, ...).
class AIPass : public InstVisitor<AIPass> {
	public:
		/// LV - result of the Live pass
		Live * LV;
		
		/// id of the pass
		params passID;

	protected:
		/// LSMT - result of the SMTpass pass
		SMTpass * LSMT;

		/// A - list of active Nodes, that still have to be computed
		std::priority_queue<Node*,std::vector<Node*>,NodeCompare> A;
		/// is_computed - remember the Nodes that don't need to be recomputed.
		/// This is used to remove duplicates in the A list.
		std::map<Node*,bool> is_computed;
	
		/// man - apron manager we use along the pass
		ap_manager_t* man;

		/// aman - manager that creates abstract values
		AbstractMan* aman;

		void loopiter(
			Node * n, 
			Abstract * &Xtemp,
			std::list<BasicBlock*> * path,
			bool &only_join,
			PathTree * const U,
			PathTree * const V);
	
		/// delete all pathtrees inside the map and clear the map
		void ClearPathtreeMap(std::map<BasicBlock*,PathTree*> & pathtree);

	public:

		AIPass (Apron_Manager_Type _man, bool use_New_Narrowing, bool _use_Threshold) : 
			LV(NULL),
			LSMT(NULL),
			unknown(false),
			NewNarrowing(use_New_Narrowing),
			use_threshold(_use_Threshold) {
				man = create_manager(_man);
				init();
			}

		AIPass () : 
			LV(NULL),
			LSMT(NULL),
			unknown(false) {
				man = create_manager(getApronManager());
				NewNarrowing = useNewNarrowing();
				use_threshold = useThreshold();
				init();
			}

		void init() {
				init_apron();
				threshold = ap_lincons1_array_make(
					ap_environment_alloc_empty(),0
				);
		}

		virtual ~AIPass () {
				ap_manager_free(man);
			}

		virtual void computeFunction(Function * F) = 0;

		/// Set to true when the analysis fails (timeout, ...)
		bool unknown;

		/// when true, apply Halbwach's narrowing
		bool NewNarrowing;

		/// when true, apply widening with threshold
		/// instead of classic widening
		bool use_threshold;

		/// initFunction - initialize the function by creating the Node
		/// objects, and computing the strongly connected components.
		void initFunction(Function * F);
		
		void printResult(Function * F);

		/// printBasicBlock - print a basicBlock on standard output
		static void printBasicBlock(BasicBlock * b);

		/// printPath - print a path on standard output
		static void printPath(std::list<BasicBlock*> path);
	
		/// computeEnv - compute the new environment of Node n, based on 
		/// its intVar and RealVar maps
		void computeEnv(Node * n);
		
		/// focuspath - path we focus on
		std::vector<BasicBlock*> focuspath;
		/// index in focuspath of the focuspath's basicblock we are working on
		unsigned focusblock;
		
		/// list of all the constraints that need to be satisfied 
		//along the path
		std::list<std::vector<ap_tcons1_array_t*>*> constraints;

		/// set of Phi variables with their associated expression, 
		/// that are defined at the last basicblock of the path
		phivar PHIvars_prime;
		
		/// set of Phi variables with their associated expression, 
		/// that are defined at the "middle" of the path 
		/// (i.e. not at the last basicblock)
		phivar PHIvars;
	
		/// computes the set of predecessors for a BasicBlock
		virtual std::set<BasicBlock*> getPredecessors(BasicBlock * b) const = 0;

		/// computes the set of Successors for a BasicBlock
		virtual std::set<BasicBlock*> getSuccessors(BasicBlock * b) const = 0;

		/// copy the elements in X_d into X_s abstract values
		/// return true iff there there some Xd values that were not equal to Xs
		bool copy_Xd_to_Xs(Function * F);

		///
		void copy_Xs_to_Xf(Function * F);
		void copy_Xf_to_Xs(Function * F);

		/// computeTransform - computes in Xtemp the polyhedra resulting from
		/// the transformation  of n->X through the path
		void computeTransform (	
			AbstractMan * aman,
			Node * n, 
			std::list<BasicBlock*> path, 
			Abstract &Xtemp);

		/// compute Seeds for Halbwach's narrowing
		/// returns true iff one ore more seeds have been found
		bool computeWideningSeed(Function * F);

		/// Basic abstract interpretation ascending iterations
		/// (iterates over the nodes, calling computeNode for each of
		/// them)
		virtual void ascendingIter(Node * n, Function * F, bool dont_reset = false);

		/// computeNode - compute and update the Abstract value of the Node n
		/// This function should update the set A of active nodes to
		/// reflect changes performed on Node n.
		virtual void computeNode(Node * n) = 0;
		
		/// Narrowing algorithm (iterates over the nodes, calling
		/// narrowNode() for each of them)
		virtual void narrowingIter(Node * n);

		/// narrowNode - apply narrowing at node n
		/// This function should update the set A of active nodes to
		/// reflect changes performed on Node n.
		virtual void narrowNode(Node * n) = 0;

		/// computeCondition - creates the constraint arrays resulting from a
		/// comparison instruction.
		bool computeCondition(CmpInst * inst, 
				bool result,
				std::vector<ap_tcons1_array_t *> * cons);

		bool computeConstantCondition(ConstantInt * inst, 
				bool result,
				std::vector<ap_tcons1_array_t*> * cons);

		bool computePHINodeCondition(PHINode * inst, 
				bool result,
				std::vector<ap_tcons1_array_t*> * cons);


		/// threshold - array of lincons we use to do widening with threshold
		/// this array is computed in computeTransform
		ap_lincons1_array_t threshold;

		// create_constraints - this function is called by computeCondition
		// it creates the constraint from its arguments and insert it into t_cons
		void create_constraints(
			ap_constyp_t constyp,
			ap_texpr1_t * expr,
			ap_texpr1_t * nexpr,
			std::vector<ap_tcons1_array_t*> * t_cons);

		void insert_env_vars_into_node_vars(ap_environment_t * env, Node * n, Value * V);

		void visitInstAndAddVarIfNecessary(Instruction &I);
		/// @{
		/// @name Visit methods
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
		/// @}
};

extern AIPass * CurrentAIpass;

#endif
