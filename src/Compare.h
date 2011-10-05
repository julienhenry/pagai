#ifndef COMPARE_H
#define COMPARE_H

#include "SMTpass.h"
#include "Node.h"
#include "Abstract.h"
#include "Analyzer.h"

using namespace llvm;

/// class that stores the results of the Compare class
class CmpResults {

	public:
		int gt;
		int lt;
		int eq;
		int un;

		CmpResults(): gt(0), lt(0), eq(0), un(0) {};
};

/// Pass that compares abstract values computed by each AI pass
class Compare : public ModulePass {

	private:
		SMTpass * LSMT;

		std::map<
			Techniques,
			std::map<Techniques,CmpResults> 
			> results;

		int compareAbstract(Abstract * A, Abstract * B);

		void compareTechniques(
			Node * n, 
			Techniques t1, 
			Techniques t2);

		void printAllResults();
		void printResults(Techniques t1, Techniques t2);
	public:
		static char ID;

		Compare();

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);

};
#endif
