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
		
		Cudd * mgr;
		std::map<BasicBlock*,int> BddVar;
		std::map<BasicBlock*,int> BddVarStart;
		BDD Bdd;
		int i;

		struct pathnode {
			std::vector<BasicBlock*> name;
			std::vector<pathnode *> next;
		};

		pathnode start;

		BDD getBDDfromBasicBlock(BasicBlock * b,std::map<BasicBlock*,int> &map);
		std::string getNodeName(BasicBlock* b, bool start);

	public:
		
		PathTree();

		~PathTree();

		void insert(std::list<BasicBlock*> path);

		void remove(std::list<BasicBlock*> path);

		void clear();

		bool exist(std::list<BasicBlock*> path);

		void DumpDotBDD(BDD graph, std::string filename);
};

#endif
