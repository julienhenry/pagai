#ifndef _PATHTREE_H
#define _PATHTREE_H

#include <list>
#include <map>
#include <vector>
#include <string>
#include "llvm/BasicBlock.h"

#include "cuddObj.hh"

using namespace llvm;

class PathTree {

	private:
		/// mgr - manager of the CUDD library 
		Cudd * mgr;

		/// BddVar - stores the index of the basicBlock in the BDD
		std::map<BasicBlock*,int> BddVar;
		/// BddVarStart - stores the index of the source basicBlock in the BDD
		std::map<BasicBlock*,int> BddVarStart;

		/// Bdd - Bdd that stores the various seen paths
		BDD Bdd;
		/// BddIndex - number of levels in the BDD
		int BddIndex;

		/// getBDDfromBasicBlock - returns the BDD node associated to a specific
		/// BasicBlock. When considering the source BasicBlock, the map is
		/// BddVarStart, else it is BddVar
		BDD getBDDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map);

		/// GetNodeName - return the name of the BasicBlock, such as it is
		/// displayed when dumping in a .dot file
		std::string getNodeName(BasicBlock* b, bool start);

	public:
		PathTree();

		~PathTree();

		/// insert - insert a path in the Bdd
		void insert(std::list<BasicBlock*> path);

		/// remove - remove a path from the Bdd
		void remove(std::list<BasicBlock*> path);

		/// clear - clear the Bdd. The result will be an empty Bdd
		void clear();

		/// exist - check if the Bdd contains the path given as argument
		bool exist(std::list<BasicBlock*> path);

		/// DumpDotBDD - dump the BDD "graph" in a .dot file. Name of the .dot
		/// file is given by the filename argument.
		void DumpDotBDD(BDD graph, std::string filename);
};

#endif
