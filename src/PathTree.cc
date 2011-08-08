#include <map>
#include <sstream>

#include "cuddObj.hh"

#include "PathTree.h"
#include "Analyzer.h"

PathTree::PathTree() {
	mgr = new Cudd(0,0);
	//mgr->makeVerbose();
	Bdd = mgr->bddZero();
	BddIndex=0;
}

PathTree::~PathTree() {
	//delete mgr;	
}

BDD PathTree::getBDDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map) {
	int n;
	if (!map.count(b)) {
		BddIndex++;
		n = BddIndex;
		map[b] = n;
	} else {
		n = map[b];	
	}
	return mgr->bddVar(n);
}

std::string PathTree::getNodeName(BasicBlock* b, bool start) {
	std::ostringstream name;
	if (start)
		name << "bs_" << b;
	else
		name << "b_" << b;
	return name.str();
}

void PathTree::DumpDotBDD(BDD graph, std::string filename) {
	std::ostringstream name;
	name << filename << ".dot";

	int n = BddVar.size() + BddVarStart.size();

	char * inames[n];
	for (std::map<BasicBlock*,int>::iterator it = BddVar.begin(), et = BddVar.end(); it != et; it++) {
		inames[it->second] = strdup(getNodeName(it->first,false).c_str());
	}
	for (std::map<BasicBlock*,int>::iterator it = BddVarStart.begin(), et = BddVarStart.end(); it != et; it++) {
		inames[it->second] = strdup(getNodeName(it->first,true).c_str());
	}

    char const* onames[] = {"B"};
    DdNode *Dds[] = {graph.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
    int result = Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            (char**) inames, (char**) onames, fp);
	fclose(fp);
}

void PathTree::insert(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());
	
	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar));
		f = f * block;
	}
	Bdd = Bdd + f;
	//DumpDotBDD(Bdd,"Bdd");
}

void PathTree::remove(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar));
		f = f * block;
	}
	Bdd = Bdd * !f;
}

void PathTree::clear() {
	Bdd = mgr->bddZero();
}

bool PathTree::exist(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	
	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlock(current, BddVarStart));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current, BddVar));
		f = f * block;
	}
	if (f <= Bdd) {
		return true;
	} else {
		return false;
	}
}
