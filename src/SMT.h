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

#include "Abstract.h"
#include "SMT_manager.h"

using namespace llvm;


class SMT : public ModulePass, public InstVisitor<SMT> {
	
	private:
		LoopInfo * LI;
		std::map<Function*,SMT_expr> rho;
		std::map<Function*,std::set<BasicBlock*>*> Pr;
		std::vector<SMT_expr> rho_components;
		std::vector<SMT_expr> instructions;

		std::string getNodeName(BasicBlock* b, bool src);
		std::string getEdgeName(BasicBlock* b1, BasicBlock* b2);
		std::string getValueName(Value * v, bool primed);
		SMT_expr getValueExpr(Value * v, std::set<Value*> ssa_defs);
		SMT_expr getValueType(Value * v);
		SMT_var getVar(Value * v, bool primed);

		void getElementFromString(	std::string name,
									bool &isEdge, 
									bool &start, 
									BasicBlock * &src, 
									BasicBlock * &dest);
		
		SMT_expr computeCondition(PHINode * inst);
		SMT_expr computeCondition(CmpInst * inst);

		SMT_expr construct_phi_ite(PHINode &I, unsigned i, unsigned n);

		void computePr(Function &F);
		
		std::map<BasicBlock*, std::set<Value*> > primed;
		std::set<Value*> exist_prime;

		void computeRhoRec(	Function &F, 
							BasicBlock * b,
							BasicBlock * dest,
							bool newPr,
							std::set<BasicBlock*> * visited);
		void computeRho(Function &F);
	public:
		static char ID;
		SMT_manager * man;

		SMT();
		~SMT();

		std::map<BasicBlock*,std::set<BasicBlock*> > Pr_succ;
		std::map<BasicBlock*,std::set<BasicBlock*> > Pr_pred;

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		//bool runOnFunction(Function &F);
		bool runOnModule(Module &M);

		std::set<BasicBlock*>* getPr(Function &F);
		SMT_expr getRho(Function &F);

		std::set<BasicBlock*> getPrPredecessors(BasicBlock * b);
		std::set<BasicBlock*> getPrSuccessors(BasicBlock * b);

		void push_context();
		void pop_context();

		SMT_expr createSMTformula(BasicBlock * source, bool narrow);
		bool SMTsolve(SMT_expr expr, std::list<BasicBlock*> * path);

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
