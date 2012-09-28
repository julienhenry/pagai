#ifndef _EXPR_H
#define _EXPR_H

#include <map>

#include "ap_global1.h"
#include "llvm/Config/config.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Constants.h"

#include "Node.h"
#include "Abstract.h"

using namespace llvm;

class Expr : public InstVisitor<Expr,ap_texpr1_t*> {

	friend class Abstract;

	private:
		ap_texpr1_t * ap_expr;

		ap_texpr1_t * create_ap_expr(Constant * val);
		ap_texpr1_t * create_ap_expr(ap_var_t var);

		Expr();

	public:

		/// common_environment - computes and returns the least common environment of
		/// two environments.
		static ap_environment_t * common_environment(
				ap_environment_t * env1,
				ap_environment_t * env2);

		/// intersect_environment - compute the intersection of the two environments.
		static ap_environment_t * intersect_environment(
				ap_environment_t * env1,
				ap_environment_t * env2);

		/// common_environment - modifies the two expression by giving them the same
		/// least common environment.
		static void common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2);


		/// get_ap_type - compute the Apron type of the LLVM Value
		/// return 0 iff the type is int or real, 1 in the other cases
		static int get_ap_type(Value * val,ap_texpr_rtype_t &ap_type);

		static void environment_print(ap_environment_t * env);
		static void texpr1_print(ap_texpr1_t * expr);
		static void tcons1_array_print(ap_tcons1_array_t * cons);

	public:


		~Expr();

		Expr(Value * val);
		Expr(ap_var_t var);
		Expr(const Expr &exp);

		Expr(ap_texpr_op_t op, Expr exp1, Expr exp2, ap_texpr_rtype_t type, ap_texpr_rdir_t round);

		// Overloaded copy assignment operator
		Expr & operator= (const Expr & exp);

		// clear all expressions stored in the internal map
		static void clear_exprs();

		// create_constraints - this function 
		// creates the constraint from its arguments and insert it into t_cons
		static void create_constraints (
			ap_constyp_t constyp,
			Expr * expr,
			Expr * nexpr,
			std::vector<ap_tcons1_array_t*> * t_cons);

		/// common_environment - modifies the two expression by giving them the same
		/// least common environment.
		static void common_environment(Expr * exp1, Expr * exp2);

		ap_environment_t * getEnv();
		ap_texpr1_t * getExpr();

		static void set_expr(Value * val, Expr exp);

		void print();

		/// @{
		/// @name Visit methods
		ap_texpr1_t * visitReturnInst (ReturnInst &I);
		ap_texpr1_t * visitBranchInst (BranchInst &I);
		ap_texpr1_t * visitSwitchInst (SwitchInst &I);
		ap_texpr1_t * visitIndirectBrInst (IndirectBrInst &I);
		ap_texpr1_t * visitInvokeInst (InvokeInst &I);
#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MINOR == 0
		ap_texpr1_t * visitUnwindInst (UnwindInst &I);
#endif
		ap_texpr1_t * visitUnreachableInst (UnreachableInst &I);
		ap_texpr1_t * visitICmpInst (ICmpInst &I);
		ap_texpr1_t * visitFCmpInst (FCmpInst &I);
		ap_texpr1_t * visitAllocaInst (AllocaInst &I);
		ap_texpr1_t * visitLoadInst (LoadInst &I);
		ap_texpr1_t * visitStoreInst (StoreInst &I);
		ap_texpr1_t * visitGetElementPtrInst (GetElementPtrInst &I);
		ap_texpr1_t * visitPHINode (PHINode &I);
		ap_texpr1_t * visitTruncInst (TruncInst &I);
		ap_texpr1_t * visitZExtInst (ZExtInst &I);
		ap_texpr1_t * visitSExtInst (SExtInst &I);
		ap_texpr1_t * visitFPTruncInst (FPTruncInst &I);
		ap_texpr1_t * visitFPExtInst (FPExtInst &I);
		ap_texpr1_t * visitFPToUIInst (FPToUIInst &I);
		ap_texpr1_t * visitFPToSIInst (FPToSIInst &I);
		ap_texpr1_t * visitUIToFPInst (UIToFPInst &I);
		ap_texpr1_t * visitSIToFPInst (SIToFPInst &I);
		ap_texpr1_t * visitPtrToIntInst (PtrToIntInst &I);
		ap_texpr1_t * visitIntToPtrInst (IntToPtrInst &I);
		ap_texpr1_t * visitBitCastInst (BitCastInst &I);
		ap_texpr1_t * visitSelectInst (SelectInst &I);
		ap_texpr1_t * visitCallInst(CallInst &I);
		ap_texpr1_t * visitVAArgInst (VAArgInst &I);
		ap_texpr1_t * visitExtractElementInst (ExtractElementInst &I);
		ap_texpr1_t * visitInsertElementInst (InsertElementInst &I);
		ap_texpr1_t * visitShuffleVectorInst (ShuffleVectorInst &I);
		ap_texpr1_t * visitExtractValueInst (ExtractValueInst &I);
		ap_texpr1_t * visitInsertValueInst (InsertValueInst &I);
		ap_texpr1_t * visitTerminatorInst (TerminatorInst &I);
		ap_texpr1_t * visitBinaryOperator (BinaryOperator &I);
		ap_texpr1_t * visitCmpInst (CmpInst &I);
		ap_texpr1_t * visitCastInst (CastInst &I);

		ap_texpr1_t * visitInstruction(Instruction &I) {
			return NULL;
		}
		/// @}
		//
		ap_texpr1_t * visitInstAndAddVar(Instruction &I);

		static void tcons1_array_deep_clear(ap_tcons1_array_t * array);
};
#endif
