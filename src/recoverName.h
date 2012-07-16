#ifndef _RECOVERNAME_H
#define _RECOVERNAME_H

#include "llvm/Module.h"
#include<map>
#include "llvm/Support/InstIterator.h"
#include "Info.h"
#include <llvm/Constants.h>
#include "llvm/IntrinsicInst.h"
#include<iostream>
#include <set>
#define LLVM_DEBUG_VERSION 720896

using namespace llvm;

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

class recoverName
{
	std::multimap<const Value*,Info*> M;
	std::map<BasicBlock*,int> BBM; //Basic Block Mapping to the starting line no. of basicblock in original code.
	void pass1(Function*F);
	void pass2(Function *F);
	bool heyPHINode(const PHINode *PHIN,std::vector<const PHINode*>& PHIvector,std::vector<Info*>& v);
	
	public:
	Info* getMDInfos(const Value* V);
	int process(Function* F);
	int getBasicBlockLineNo(BasicBlock* BB);
};

#endif