#ifndef _PATHTREE_H
#define _PATHTREE_H

#include <list>
#include <map>
#include <vector>
#include <string>
#include "llvm/BasicBlock.h"

#include "cuddObj.hh"

#include "SMTpass.h"

using namespace llvm;

/// BDD representing sets of paths in the graph
class PathTree {

	private:
		/// manager of the CUDD library 
		Cudd * mgr;
		
		/// @{
		/// variables needed by some methods
		DdNode * background;
		DdNode * zero;
		/// @}

		/// stores the index of the basicBlock in the BDD
		std::map<BasicBlock*,int> BddVar;
		/// stores the index of the source basicBlock in the BDD
		std::map<BasicBlock*,int> BddVarStart;

		std::map<int, BasicBlock*> levels;

		BDD computef(std::list<BasicBlock*> path);

		/// Bdd that stores the various seen paths
		BDD * Bdd;

		/// Bdd that stores the paths that need to be added in Bdd
		/// in the next step
		BDD * Bdd_prime;

		/// number of levels in the BDD
		int BddIndex;

		BDD getBDDfromBddIndex(int n);
		/// returns the BDD node associated to a specific
		/// BasicBlock. When considering the source BasicBlock, the map is
		/// BddVarStart, else it is BddVar
		BDD getBDDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map, int &n);

		/// returns the name of the basicBlock associated
		/// to the level i of the Bdd.
		/// If smt != NULL, this name is exactly the same as the one 
		/// used in the SMTpass pass
		const std::string getStringFromLevel(
			int i,
			SMTpass * smt = NULL);

		void createBDDVars(BasicBlock * Start, std::set<BasicBlock*> * Pr, std::map<BasicBlock*,int> &map, std::set<BasicBlock*> * seen, bool start = false);

		/// dump the BDD "graph" in a .dot file. Name of the .dot
		/// file is given by the filename argument.
		void DumpDotBDD(BDD graph, std::string filename);

	public:
		PathTree(BasicBlock * Start);

		~PathTree();

		/// insert a path in the Bdd
		void insert(std::list<BasicBlock*> path, bool primed = false);

		/// remove a path from the Bdd
		void remove(std::list<BasicBlock*> path, bool primed = false);

		/// clear the Bdd. The result will be an empty Bdd
		void clear(bool primed = false);

		/// check if the Bdd contains the path given as argument
		bool exist(std::list<BasicBlock*> path, bool primed = false);

		/// merge the two Bdds into Bdd. Bdd_prime is cleared
		void mergeBDD();

		bool isZero(bool primed = false);

		/// dump the graph
		void DumpDotBDD(std::string filename, bool prime);

		/// generate the SMTpass formula associated to the Bdd
		SMT_expr generateSMTformula(
			SMTpass * smt, bool neg = false);
};
#endif
