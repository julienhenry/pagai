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
 * \class compare_1
 * \brief class for comparing Info objects
 */
class compare_1
{
	public:
		bool operator()(Info* x,Info* y)
		{
			if(x->getLineNo() == y->getLineNo())
			{
				int compareVal=(x->getName()).compare(y->getName());
				if(compareVal)
					return (compareVal < 0);
				else
				{
					compareVal=(x->getType()).compare(y->getType());
					return (compareVal < 0);
				}
			}
			else
				return (x->getLineNo() < y->getLineNo());
		}
};

/**
 * \class recoverName
 * \brief recover the names of the variables from the source code
 */
class recoverName {
	private :
		static void pass1(Function *F);
		static void pass2(Function *F);
		static bool evaluatePHINode(
				const PHINode *PHIN,
				std::vector<const PHINode*>& PHIvector,
				std::vector<Info*>& v);

	public:
		static Info* getMDInfos(const Value* V);
		static int process(Function* F);
		static int getBasicBlockLineNo(BasicBlock* BB);
		static int getBasicBlockColumnNo(BasicBlock* BB);
		static std::string getSourceFileName();
};

#endif
