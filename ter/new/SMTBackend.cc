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

#include "SMTBackend.h"
#include "SMTBackend_decl.h"

// #define COMMENT

using namespace llvm;
using namespace std;

char SMTBackend::ID = 0;

const char *SMTBackend::getPassName() const {
	return "not implemented";
}

// void SMTBackend::getAnalysisUsage(AnalysisUsage &AU) const{}

bool SMTBackend::runOnModule(Module &M){

	Out << "(benchmark VERIMAG\n";
	Out << ":category { random }\n";
	Out << ":status unknown\n";
	Out << ":source { VERIMAG, Synchrone } \n\n"; 

	SMTBackend_decl * declaration = new SMTBackend_decl(Out,&locals_name);
	// this first pass creates the definitions of variables
	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function* F = &*mIt;
		declaration->printFunction(F);
	}
	// creation of formula
	Out << ":formula\n";
	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		Function* F = &*mIt;
		printFunction(F);
	}

	Out << ")\n";
	return 0;
}

void SMTBackend::printFunction(Function* F) {
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
		// Out << "Basic block (name=" << i->getName() << ") has "
		//        << i->size() << " instructions.\n";
		printBasicBlock(i);
	}
}

void SMTBackend::printBasicBlock(BasicBlock* blk) {

	Out << "(and " << blk->getName() << "\n" ;
	for (BasicBlock::iterator i = blk->begin(), e = blk->end();
			i != e; ++i) {
		visit(*i);
		// Out << *i << "\n";
	}
	Out << ")\n";
}

string GetConstantName(const Constant * Value) {
	string Name;
	stringstream ss;

	if (const ConstantInt * Const = dyn_cast < ConstantInt > (Value)) {
		// Out << "This is a ConstantInt\n";
		uint64_t Val = Const->getLimitedValue();
		ss << Val;
		Name =  ss.str();
	}

	if (Name.empty()) {
		Name = "undef_Constant";
	}
	return Name;
}

string SMTBackend::GetUserName(const User * Value) {

	string Name;
	stringstream ss;

	if (const Constant * Const = dyn_cast < Constant > (Value)) {
		return GetConstantName(Const);
	}

	if (const Instruction * Inst = dyn_cast < Instruction > (Value)) {
		Name = locals_name[Inst];
		if (Name.empty()) {
			Name = "undef_Instruction";
		}
	}

	if (Name.empty()) {
		Name = "undef_User";
	}
	return Name;
}


string SMTBackend::GetValueName(const Value * Operand) {

	string Name = Operand->getNameStr();

	if (const User * Const = dyn_cast < User > (Operand)) {
		return GetUserName(Const);
	}

	if (Name.empty()) {
		Name = "undef_Value";
	}

	return Name;

}

void SMTBackend::visitReturnInst (ReturnInst &I) {}
void SMTBackend::visitBranchInst (BranchInst &I) {}
void SMTBackend::visitSwitchInst (SwitchInst &I){}
void SMTBackend::visitIndirectBrInst (IndirectBrInst &I) {}
void SMTBackend::visitInvokeInst (InvokeInst &I) {}
void SMTBackend::visitUnwindInst (UnwindInst &I) {}
void SMTBackend::visitUnreachableInst (UnreachableInst &I) {}
void SMTBackend::visitICmpInst (ICmpInst &I) {}
void SMTBackend::visitFCmpInst (FCmpInst &I) {}
void SMTBackend::visitAllocaInst (AllocaInst &I) {}
void SMTBackend::visitLoadInst (LoadInst &I) {}
void SMTBackend::visitStoreInst (StoreInst &I) {}
void SMTBackend::visitGetElementPtrInst (GetElementPtrInst &I) {}
void SMTBackend::visitPHINode (PHINode &I) {}
void SMTBackend::visitTruncInst (TruncInst &I) {}
void SMTBackend::visitZExtInst (ZExtInst &I) {}
void SMTBackend::visitSExtInst (SExtInst &I) {}
void SMTBackend::visitFPTruncInst (FPTruncInst &I) {}
void SMTBackend::visitFPExtInst (FPExtInst &I) {}
void SMTBackend::visitFPToUIInst (FPToUIInst &I) {}
void SMTBackend::visitFPToSIInst (FPToSIInst &I) {}
void SMTBackend::visitUIToFPInst (UIToFPInst &I) {}
void SMTBackend::visitSIToFPInst (SIToFPInst &I) {}
void SMTBackend::visitPtrToIntInst (PtrToIntInst &I) {}
void SMTBackend::visitIntToPtrInst (IntToPtrInst &I) {}
void SMTBackend::visitBitCastInst (BitCastInst &I) {}
void SMTBackend::visitSelectInst (SelectInst &I) {}
void SMTBackend::visitCallInst(CallInst &I) {}
void SMTBackend::visitVAArgInst (VAArgInst &I) {}
void SMTBackend::visitExtractElementInst (ExtractElementInst &I) {}
void SMTBackend::visitInsertElementInst (InsertElementInst &I) {}
void SMTBackend::visitShuffleVectorInst (ShuffleVectorInst &I) {}
void SMTBackend::visitExtractValueInst (ExtractValueInst &I) {}
void SMTBackend::visitInsertValueInst (InsertValueInst &I) {}
void SMTBackend::visitTerminatorInst (TerminatorInst &I) {}
void SMTBackend::visitBinaryOperator (BinaryOperator &I) {}
void SMTBackend::visitCmpInst (CmpInst &I) {}
void SMTBackend::visitCastInst (CastInst &I) {}



