#include <iostream>
#include <sstream>

#include "llvm/CallingConv.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/TypeSymbolTable.h"
#include "llvm/Intrinsics.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/InlineAsm.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/ConstantsScanner.h"
#include "llvm/Analysis/FindUsedTypes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/System/Host.h"
#include "llvm/Config/config.h"

#include "SMTBackend_decl.h"

// #define COMMENT

using namespace llvm;
using namespace std;

int SMTBackend_decl::num_local = 0;

const char *SMTBackend_decl::getPassName() const {
	return "not implemented";
}

void SMTBackend_decl::printFunction(Function* F) {
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		Out << ":extrapreds ((" << i->getName() << "))\n";
	}
	Out << "\n";
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		printBasicBlock(i);
	}
}

void SMTBackend_decl::printBasicBlock(BasicBlock* blk) {

	for (BasicBlock::iterator i = blk->begin(), e = blk->end();
	     i != e; ++i) {
		visit(*i);
	}
}


void SMTBackend_decl::visitReturnInst (ReturnInst &I) {}
void SMTBackend_decl::visitBranchInst (BranchInst &I) {}
void SMTBackend_decl::visitSwitchInst (SwitchInst &I){}
void SMTBackend_decl::visitIndirectBrInst (IndirectBrInst &I) {}
void SMTBackend_decl::visitInvokeInst (InvokeInst &I) {}
void SMTBackend_decl::visitUnwindInst (UnwindInst &I) {}
void SMTBackend_decl::visitUnreachableInst (UnreachableInst &I) {}
void SMTBackend_decl::visitICmpInst (ICmpInst &I) {}
void SMTBackend_decl::visitFCmpInst (FCmpInst &I) {}
void SMTBackend_decl::visitAllocaInst (AllocaInst &I) {}
void SMTBackend_decl::visitLoadInst (LoadInst &I) {}
void SMTBackend_decl::visitStoreInst (StoreInst &I) {}
void SMTBackend_decl::visitGetElementPtrInst (GetElementPtrInst &I) {}
void SMTBackend_decl::visitPHINode (PHINode &I) {}
void SMTBackend_decl::visitTruncInst (TruncInst &I) {}
void SMTBackend_decl::visitZExtInst (ZExtInst &I) {}
void SMTBackend_decl::visitSExtInst (SExtInst &I) {}
void SMTBackend_decl::visitFPTruncInst (FPTruncInst &I) {}
void SMTBackend_decl::visitFPExtInst (FPExtInst &I) {}
void SMTBackend_decl::visitFPToUIInst (FPToUIInst &I) {}
void SMTBackend_decl::visitFPToSIInst (FPToSIInst &I) {}
void SMTBackend_decl::visitUIToFPInst (UIToFPInst &I) {}
void SMTBackend_decl::visitSIToFPInst (SIToFPInst &I) {}
void SMTBackend_decl::visitPtrToIntInst (PtrToIntInst &I) {}
void SMTBackend_decl::visitIntToPtrInst (IntToPtrInst &I) {}
void SMTBackend_decl::visitBitCastInst (BitCastInst &I) {}
void SMTBackend_decl::visitSelectInst (SelectInst &I) {}
void SMTBackend_decl::visitCallInst(CallInst &I) {}
void SMTBackend_decl::visitVAArgInst (VAArgInst &I) {}
void SMTBackend_decl::visitExtractElementInst (ExtractElementInst &I) {}
void SMTBackend_decl::visitInsertElementInst (InsertElementInst &I) {}
void SMTBackend_decl::visitShuffleVectorInst (ShuffleVectorInst &I) {}
void SMTBackend_decl::visitExtractValueInst (ExtractValueInst &I) {}
void SMTBackend_decl::visitInsertValueInst (InsertValueInst &I) {}
void SMTBackend_decl::visitTerminatorInst (TerminatorInst &I) {}
void SMTBackend_decl::visitBinaryOperator (BinaryOperator &I) {}
void SMTBackend_decl::visitCmpInst (CmpInst &I) {}
void SMTBackend_decl::visitCastInst (CastInst &I) {}


