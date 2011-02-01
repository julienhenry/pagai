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
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Support/Mangler.h"
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


// Opcode Implementations
void SMTBackend::visitReturnInst(ReturnInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitReturnInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitBranchInst(BranchInst &I){
#ifdef COMMENT
	Out << ";; " << I << "\n";
#endif
	if (I.isConditional()) {
		Out << "(ite "
		    << GetValueName(I.getCondition()) << " "
		    << I.getSuccessor(0)->getName() << " "
		    << I.getSuccessor(1)->getName()
		    << ")\n";
	} else {
		Out << "(" << I.getSuccessor(0)->getName() << ")";
	}
}

void SMTBackend::visitSwitchInst(SwitchInst &I){
#ifdef COMMENT
	Out << ";; switchInst" << "\n";
#endif
	string cond = GetValueName(I.getCondition());

	for (unsigned i=1;i<I.getNumCases();i++) {
		Out << "(ite (= " << cond << " "  
		    << GetConstantName(I.getCaseValue(i)) << ") "
		    << I.getSuccessor(i)->getName() << " ";
	}
	Out << I.getDefaultDest()->getName();
	for (unsigned i=1;i<I.getNumCases();i++) {
		Out << ")";
	}
	Out << "\n";
}

void SMTBackend::visitBinaryOperator(BinaryOperator &I){
#ifdef COMMENT
	Out << ";; " << I << "\n"; 
#endif
	Out << "(= " << locals_name[&I] << " (";
	switch (I.getOpcode()) {
	case Instruction::Add:
	case Instruction::FAdd:
		Out << "+ ";
		break;
	case Instruction::Sub:
	case Instruction::FSub:
		Out << "- ";
		break;
	case Instruction::Mul:
	case Instruction::FMul:
		Out << "* ";
		break;
	case Instruction::URem:
	case Instruction::SRem:
	case Instruction::FRem:
		Out << "% ";
		break;
	case Instruction::UDiv:
	case Instruction::SDiv:
	case Instruction::FDiv:
		Out << "/ ";
		break;
	case Instruction::And:
		Out << "and ";
		break;
	case Instruction::Or:
		Out << "or ";
		break;
	case Instruction::Xor:
		Out << "xor ";
		break;
	case Instruction::Shl:
		Out << "<< ";
		break;
	case Instruction::LShr:
	case Instruction::AShr:
	case Instruction::BinaryOpsEnd:
		break;
	}
	
	// Out << GetValueName(I.getOperand(0)) << ")\n";
	Out << GetValueName(I.getOperand(0)) << " " << GetValueName(I.getOperand(1)) << "))\n";
	
}

void SMTBackend::visitICmpInst(ICmpInst &I){
#ifdef COMMENT
	Out << ";; " << I << "\n";
#endif
	Out << "(= " << locals_name[&I] << " (";
	switch (I.getPredicate()) {
	case CmpInst::ICMP_EQ:
		Out << "=";
		break;
	case CmpInst::ICMP_NE:
		Out << "#";
		break;
	case CmpInst::ICMP_UGT:
		Out << ">";
		break;
	case CmpInst::ICMP_UGE:
		Out << ">=";
		break;
	case CmpInst::ICMP_ULT:
		Out << "<";
		break;
	case CmpInst::ICMP_ULE:
		Out << "<=";
		break;
	case CmpInst::ICMP_SGT:
		Out << ">";
		break;
	case CmpInst::ICMP_SGE:
		Out << ">=";
		break;
	case CmpInst::ICMP_SLT:
		Out << "<";
		break;
	case CmpInst::ICMP_SLE:
		Out << "<=";
		break;
	default:
		Out << "undef_predicate";
	}
	
	Out << " " << GetValueName(I.getOperand(0)) << " "
	    << GetValueName(I.getOperand(1)) << "))\n";
}

void SMTBackend::visitFCmpInst(FCmpInst &I){
#ifdef COMMENT
	Out << ";; " << I << "\n";
#endif
	Out << "(= " << locals_name[&I] << " (";
	switch (I.getPredicate()) {
	case CmpInst::FCMP_FALSE:
		Out << "false";
		break;
	case CmpInst::FCMP_OEQ:
		Out << "=";
		break;
	case CmpInst::FCMP_OGT:
		Out << ">";
		break;
	case CmpInst::FCMP_OGE:
		Out << ">=";
		break;
	case CmpInst::FCMP_OLT:
		Out << "<";
		break;
	case CmpInst::FCMP_OLE:
		Out << "<=";
		break;
	case CmpInst::FCMP_ONE:
		Out << "#";
		break;
	case CmpInst::FCMP_ORD:
		Out << "true";
		break;
	case CmpInst::FCMP_UNO:
		Out << "false";
		break;
	case CmpInst::FCMP_UEQ:
		Out << "=";
		break;
	case CmpInst::FCMP_UGT:
		Out << ">";
		break;
	case CmpInst::FCMP_UGE:
		Out << ">=";
		break;
	case CmpInst::FCMP_ULT:
		Out << "<";
		break;
	case CmpInst::FCMP_ULE:
		Out << "<=";
		break;
	case CmpInst::FCMP_UNE:
		Out << "#";
		break;
	case CmpInst::FCMP_TRUE:
		Out << "true";
		break;
	default:
		Out << "undef_predicate";
	}

	Out << " " << GetValueName(I.getOperand(0)) << " "
	    << GetValueName(I.getOperand(1)) << "))\n";
}

void SMTBackend::visitAllocationInst(AllocationInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitAllocationInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitFreeInst(FreeInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitFreeInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitLoadInst(LoadInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitLoadInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitStoreInst(StoreInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitStoreInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitGetElementPtrInst(GetElementPtrInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitGetElementPtrInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitPHINode(PHINode &I){
#ifdef COMMENT
	Out << ";; " << I << "\n";
#endif
	// only one IncomingBlock is true
	Out << "(or ";
	for (unsigned i=0;i<I.getNumIncomingValues();i++) {
		Out << I.getIncomingBlock(i)->getName();
		if (i < I.getNumIncomingValues() - 1)
			Out << " ";
	}
	Out << ")\n";
	
	for (unsigned i=0;i<I.getNumIncomingValues();i++) {
		Out << "(ite ";
		Out << I.getIncomingBlock(i)->getName() << " "
		    << GetValueName(I.getIncomingValue(i));
		if (i < I.getNumIncomingValues()-1)
			Out << " ";
	}
	for (unsigned i=0;i<I.getNumIncomingValues();i++) {
		Out << ")";
	}
	Out << "\n";
}

void SMTBackend::visitTruncInst(TruncInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitTruncInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitZExtInst(ZExtInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitZExtInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitSExtInst(SExtInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitSExtInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitFPTruncInst(FPTruncInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitFPTruncInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitFPExtInst(FPExtInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitFPExtInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitUIToFPInst(UIToFPInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitUIToFPInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitSIToFPInst(SIToFPInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitSIToFPInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitFPToUIInst(FPToUIInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitFPToUIInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitFPToSIInst(FPToSIInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitFPToSIInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitPtrToIntInst(PtrToIntInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitPtrToIntInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitIntToPtrInst(IntToPtrInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitIntToPtrInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitBitCastInst(BitCastInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitBitCastInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitSelectInst(SelectInst &I){
#ifdef COMMENT
	Out << ";; " << I << "\n";
#endif
	
	Out << "(= " << locals_name[&I] << " (ite "
	    << GetValueName(I.getCondition()) << " "
	    << GetValueName(I.getTrueValue()) << " "
	    << GetValueName(I.getFalseValue())
	    << "))\n";
}
  
void SMTBackend::visitCallSite(CallSite CS){
	Instruction* I = CS.getInstruction();
#ifdef COMMENT
	Out << ";; " << *I << "\n";
#endif
	ostringstream oss;
	oss << *(CS.getCalledFunction()) << "\n";
	if (oss.str().compare(1,22,"declare void @__assert") == 0) {
		// this is an assert function
		// we could try to verify it

		// NOT IMPLEMENTED
	}

	switch (CS.getType()->getTypeID()) {
	case Type::VoidTyID:
		return;
	default:	// NOT IMPLEMENTED
		Out << "(= " << locals_name[I] << " " <<  locals_name[I]
		    << ")\n";
		break;
	}
}

void SMTBackend::visitUnwindInst(UnwindInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitUnwindInst\n;; " << I << "\n";
#endif
}

void SMTBackend::visitUnreachableInst(UnreachableInst &I){
#ifdef COMMENT
	Out << ";; UnreachableInst : " << I << "\n";
#endif
	Out << "(false)\n";
}
 
void SMTBackend::visitShl(BinaryOperator &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitShl\n;; " << I << "\n";
#endif
}

void SMTBackend::visitLShr(BinaryOperator &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitLShr\n;; " << I << "\n";
#endif
}

void SMTBackend::visitAShr(BinaryOperator &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitAShr\n;; " << I << "\n";
#endif
}
 
void SMTBackend::visitVAArgInst(VAArgInst &I){
#ifdef COMMENT
	Out << ";; NOT IMPLEMENTED : visitVAArgInst\n;; " << I << "\n";
#endif
}

