#ifndef _PATHTREE_H
#define _PATHTREE_H

#include <list>
#include <map>
#include <vector>
#include <string>
#include "llvm/BasicBlock.h"

#include "cuddObj.hh"

#include "SMT.h"

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

		/// Bdd that stores the various seen paths
		BDD * Bdd;

		/// Bdd that stores the paths that need to be added in Bdd
		/// in the next step
		BDD * Bdd_prime;

		/// number of levels in the BDD
		int BddIndex;

		/// returns the BDD node associated to a specific
		/// BasicBlock. When considering the source BasicBlock, the map is
		/// BddVarStart, else it is BddVar
		BDD getBDDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map);

		/// returns a string that names the basicblock. 
		/// If smt != NULL, this name is exactly the same as the one 
		/// used in the SMT pass
		const std::string getNodeName(
			BasicBlock* b, 
			bool src,
			SMT * smt = NULL) const;

		/// returns the name of the basicBlock associated
		/// to the level i of the Bdd.
		/// If smt != NULL, this name is exactly the same as the one 
		/// used in the SMT pass
		const std::string getStringFromLevel(
			int i,
			SMT * smt = NULL);

	public:
		PathTree();

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

		/// dump the BDD "graph" in a .dot file. Name of the .dot
		/// file is given by the filename argument.
		void DumpDotBDD(BDD graph, std::string filename);

		/// generate the SMT formula associated to the Bdd
		SMT_expr generateSMTformula(
			SMT * smt);
};
#endif
