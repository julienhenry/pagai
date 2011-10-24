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

		/// number of levels in the ADD
		int AddIndex;

		/// get the ADD node associated to a specific basicblock	
		/// the map should be AddVar or AddVarSource, depending if we consider
		/// the starting point or not
		ADD getADDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map);

		/// Add that stores the various seen paths
		std::map<int,ADD*> Add;

		/// insert a path in the Bdd
		void insert(std::list<BasicBlock*> path, int start);

		/// remove a path from the Bdd
		void remove(std::list<BasicBlock*> path, int start);

		/// check if the Bdd contains the path given as argument
		bool exist(std::list<BasicBlock*> path, int start);
	
		/// get the actual value of sigma stored in the BDD for sigma(path,start)
		int getActualValue(std::list<BasicBlock*> path, int start);

		/// set the value of sigma(path,start)
		void setActualValue(std::list<BasicBlock*> path, int start, int value);

		//const std::string getNodeName(BasicBlock* b, bool src) const;
		
		void DumpDotADD(ADD graph, std::string filename);

		bool isZero(int start);

	public:
		
		Sigma();
		~Sigma();

		/// clear the Add. The result will be an empty Add
		void clear();

		int getSigma(
			std::list<BasicBlock*> path, 
			int start,
			Abstract * Xtemp,
			params passID,
			bool source);

};
#endif
