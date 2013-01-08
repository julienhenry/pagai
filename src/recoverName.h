/**
 * \file recoverName.h
 * \brief Declaration of the recoverName class
 * \author Rahul Nanda, Julien Henry
 */
#ifndef _RECOVERNAME_H
#define _RECOVERNAME_H

#include <set>
#include<map>

#include "llvm/Module.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Constants.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Support/Dwarf.h"

#include "Info.h"

#define LLVM_DEBUG_VERSION LLVMDebugVersion

using namespace llvm;

/**
 * \class compare_Info
 * \brief class for comparing Info objects
 */
class compare_Info
{
	public:
		bool operator()(Info x,Info y) {
			return (x.Compare(y) == -1);
		}
};

/**
 * \class recoverName
 * \brief recover the names of the variables from the source code
 */
class recoverName {
	private :
		static void pass1(Function *F);
		
		static MDNode * get_DW_TAG_lexical_block(MDNode * MD);
		static MDNode * get_DW_TAG_subprogram(MDNode * MD);
		static MDNode * get_DW_TAG_file_type(MDNode * MD);

		
		static Info resolveMetDescriptor(MDNode* md);

		static void update_line_column(Instruction * I, unsigned & line, unsigned & column);
		static void print_set(std::set<Info,compare_Info> * s);

		static std::set<Info,compare_Info> getPossibleMappings(const Value * V, std::set<const Value *> * seen);

	public:
		static Info getMDInfos(const Value* V);
		static int process(Function* F);
		static int getFunctionLineNo(Function* F);
		static int getBasicBlockLineNo(BasicBlock* BB);
		static int getBasicBlockColumnNo(BasicBlock* BB);
		static std::string getSourceFileName(Function * F);
		static std::string getSourceFileDir(Function * F);
		static Instruction * getFirstMetadata(Function * F);
		static Instruction * getFirstMetadata(BasicBlock * b);
		static bool hasMetadata(Module * M);
		static bool hasMetadata(Function * F);
		static bool hasMetadata(BasicBlock * b);
		static bool is_readable(Function * F);
};

#endif
