#ifndef SMT_H
#define SMT_H

#include <map>
#include <vector>
#include <list>
#include <set>

#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Analysis/LoopInfo.h"

#include "Analyzer.h"
#include "Abstract.h"
#include "SMT_manager.h"

using namespace llvm;


class SMT : public ModulePass, public InstVisitor<SMT> {
	
	private:
		LoopInfo * LI;

		int nundef;

		/// rho - stores the rho formula associated to each function
		std::map<Function*,SMT_expr> rho;
		/// Pr - associate to each formula its set of Pr nodes
		std::map<Function*,std::set<BasicBlock*>*> Pr;

		/// Pr_succ - associate to each basicBlock its successors in Pr
		std::map<BasicBlock*,std::set<BasicBlock*> > Pr_succ;
		/// Pr_succ - associate to each basicBlock its predecessors in Pr
		std::map<BasicBlock*,std::set<BasicBlock*> > Pr_pred;

		/// rho_components - when constructing rho, we use this vector
		std::vector<SMT_expr> rho_components;

		/// instructions - when constructing instruction-related SMT formula, we
		/// use this vector
		std::vector<SMT_expr> instructions;

		/// these following methods are used to create a variable name for
		//edges, nodes, values, undeterministic choices, ...
		const std::string getUndeterministicChoiceName(Value * v);
		const std::string getEdgeName(BasicBlock* b1, BasicBlock* b2);
		const std::string getValueName(Value * v, bool primed);

		/// getValueExpr - get the expression associated to a value
		SMT_expr getValueExpr(Value * v, std::set<Value*> ssa_defs);

		/// getValueType - return the SMT type of the value
		SMT_type getValueType(Value * v);

		/// getVar - function to use for getting a variable from a value
		SMT_var getVar(Value * v, bool primed);

		/// getElementFromString - take a string name as input, and find if it
		/// is the name of an edge or a node
		/// if it is an edge :
		///  - src becomes the source of the edge
		///  - dest becomes the destination of the edge
		/// if it is a Node :
		///  - src becomes the basicblock of this node
		///  - start is true iff the block is a start Node
		void getElementFromString(	std::string name,
									bool &isEdge, 
									bool &start, 
									BasicBlock * &src, 
									BasicBlock * &dest);
		
		/// computeCondition - compute and return the expression associated to a
		/// condition
		SMT_expr computeCondition(PHINode * inst);
		SMT_expr computeCondition(CmpInst * inst);

		/// construct_phi_ite - called by visitPHINode
		SMT_expr construct_phi_ite(PHINode &I, unsigned i, unsigned n);

		/// computePr - compute the set Pr for a function
		void computePr(Function &F);
	
		/// primed - remember which value needs to be primed in each basicblock
		std::map<BasicBlock*, std::set<Value*> > primed;

		/// computeRhoRec - recursive function called by computeRho
		void computeRhoRec(	Function &F, 
							BasicBlock * b,
							BasicBlock * dest,
							bool newPr,
							std::set<BasicBlock*> * visited);
		void computeRho(Function &F);


	public:
		static char ID;

		/// man - manager (Microsoft z3 or Yices)
		SMT_manager * man;

		SMT();
		~SMT();

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);

		void reset_SMTcontext();

		/// getPr - get the set Pr. The set Pr is computed only once
		std::set<BasicBlock*>* getPr(Function &F);
		/// getRho - get the SMT formula Rho. Rho is computed only once
		SMT_expr getRho(Function &F);

		/// getPrPredecessors - returns a set containing all the predecessors of
		/// b in Pr
		std::set<BasicBlock*> getPrPredecessors(BasicBlock * b);
		/// getPrPredecessors - returns a set containing all the successors of
		/// b in Pr
		std::set<BasicBlock*> getPrSuccessors(BasicBlock * b);

		/// push_context - push the context of the SMT manager
		void push_context();
		/// pop_context - pop the context of the SMT manager
		void pop_context();
		
		/// createSMTformula - compute and return the SMT formula associated to
		/// the BasicBlock source, as described in the paper
		/// if we are in narrowing phase, narrow has to be true, else false
		/// We can cunjunct this formula with an SMT_expr formula given as
		/// parameter of the function: constraint
		SMT_expr createSMTformula(
			BasicBlock * source, 
			bool narrow, 
			Techniques t,
			SMT_expr constraint = NULL);

		/// SMTsolve - solve the SMT expression expr and return true iff expr is
		/// satisfiable. In this case, path containts the path extracted from
		/// the model
		int SMTsolve(SMT_expr expr, std::list<BasicBlock*> * path);

		/// gets the name of the node associated to a specific basicblock
		const std::string getNodeName(BasicBlock* b, bool src);

		/// XToSmt - transform an apron object of type X into an SMT
		/// expression
		SMT_expr texpr1ToSmt(ap_texpr1_t texpr);
		SMT_expr linexpr1ToSmt(BasicBlock * b, ap_linexpr1_t linexpr, bool &integer);
		SMT_expr scalarToSmt(ap_scalar_t * scalar, bool integer, bool &iszero);
		SMT_expr tcons1ToSmt(ap_tcons1_t tcons);
		SMT_expr lincons1ToSmt(BasicBlock * b, ap_lincons1_t lincons);
		SMT_expr AbstractToSmt(BasicBlock * b, Abstract * A);

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
