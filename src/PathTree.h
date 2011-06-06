#ifndef _PATHTREE_H
#define _PATHTREE_H

#include <list>
#include <vector>
#include "llvm/BasicBlock.h"

using namespace llvm;

class PathTree {

	private:

		struct pathnode {
			std::vector<BasicBlock*> name;
			std::vector<pathnode *> next;
		};

		pathnode start;
	public:
		
		PathTree();

		~PathTree();

		void insert(std::list<BasicBlock*> path);

		void clear();

		bool exist(std::list<BasicBlock*> path);
};

#endif
