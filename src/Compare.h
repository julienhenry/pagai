
#ifndef COMPARE_H
#define COMPARE_H

#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"

#include "SMT.h"
#include "Abstract.h"
#include "AbstractGopan.h"

using namespace llvm;


class Compare : public ModulePass {

	private:
	SMT * LSMT;
	public:
	static char ID;

	Compare();

	const char * getPassName() const;
	virtual void getAnalysisUsage(AnalysisUsage &AU) const;
	virtual bool runOnModule(Module &M);

	int compareAbstract(Abstract * A, AbstractGopan * B);
};
#endif
