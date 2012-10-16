/**
 * \file Compare.h
 * \brief Declaration of the Compare pass
 * \author Julien Henry
 */
#ifndef COMPARE_H
#define COMPARE_H

#include "llvm/Module.h"
#include "llvm/Pass.h"

#include "SMTpass.h"
#include "Node.h"
#include "Abstract.h"
#include "Analyzer.h"
#include "Debug.h"

using namespace llvm;

/**
 * \class CmpResult
 * \brief class that stores the results of the Compare class
 */
class CmpResults {

	public:
		int gt;
		int lt;
		int eq;
		int un;

		CmpResults(): gt(0), lt(0), eq(0), un(0) {};
};

/**
 * \class Compare
 * \brief Pass that compares abstract values computed by each AI pass
 */
class Compare : public ModulePass {

	private:
		std::vector<Techniques> ComparedTechniques;

	protected:
		SMTpass * LSMT;

		std::map<
			Techniques,
			std::map<Techniques,CmpResults> 
			> results;

		std::map<Techniques, sys::TimeValue *> Time;

		// count the number of warnings emitted by each technique
		std::map<Techniques,int> Warnings;

		int compareAbstract(Abstract * A, Abstract * B);

		void compareTechniques(
			Node * n, 
			Techniques t1, 
			Techniques t2);

		void CompareTechniquesByPair(Node * n);
		void PrintResultsByPair();

		void ComputeTime(Techniques t, Function * F);
		void CountNumberOfWarnings(Techniques t, Function * F);
		void printTime(Techniques t);
		void printWarnings(Techniques t);

		void printAllResults();
		void printResults(Techniques t1, Techniques t2);
	public:
		static char ID;

		Compare(std::vector<enum Techniques> * T);
		Compare();

		const char * getPassName() const;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool runOnModule(Module &M);

};
#endif
