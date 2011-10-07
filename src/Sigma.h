#ifndef SIGMA_H
#define SIGMA_H

#include <list>
#include <map>

#include "llvm/BasicBlock.h"

#include "cuddObj.hh"

using namespace llvm;

class Sigma {

	private:
		/// manager of the CUDD library 
		Cudd * mgr;

		/// @{
		/// variables needed by some methods
		DdNode * background;
		DdNode * zero;
		/// @}

		/// stores the index of the basicBlock in the ADD
		std::map<BasicBlock*,int> AddVar;
		/// stores the index of the source basicBlock in the ADD
		std::map<BasicBlock*,int> AddVarSource;
		/// stores the index of the start index in the ADD
		std::map<int,int> AddVarIndex;

		/// number of levels in the ADD
		int AddIndex;

		
		ADD getADDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map);

		ADD getADDfromStartIndex(int start);

		/// Add that stores the various seen paths
		ADD * Add;

		/// insert a path in the Bdd
		void insert(std::list<BasicBlock*> path, int start);

		/// remove a path from the Bdd
		void remove(std::list<BasicBlock*> path, int start);

		/// check if the Bdd contains the path given as argument
		bool exist(std::list<BasicBlock*> path, int start);
		
		int getActualValue(std::list<BasicBlock*> path, int start);
		void setActualValue(std::list<BasicBlock*> path, int start, int value);

	public:
		
		Sigma();
		~Sigma();

		/// clear the Bdd. The result will be an empty Bdd
		void clear();


		int getSigma(std::list<BasicBlock*> path, int start);

		bool isZero();
};
#endif
