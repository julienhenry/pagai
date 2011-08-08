#include <map>
#include <sstream>

#include "cuddObj.hh"

#include "PathTree.h"
#include "Analyzer.h"

int k;
PathTree::PathTree() {
	mgr = new Cudd(0,0);
	//mgr->makeVerbose();
	Bdd = mgr->bddZero();
	i=0;
	k=0;
}

PathTree::~PathTree() {
	//delete mgr;	
}

BDD PathTree::getBDDfromBasicBlockStart(BasicBlock * b) {
	int n;
	if (!BddVarStart.count(b)) {
		i++;
		n = i;
		BddVarStart[b] = n;
	} else {
		n = BddVarStart[b];	
	}
	return mgr->bddVar(i);
}

BDD PathTree::getBDDfromBasicBlock(BasicBlock * b) {
	int n;
	if (!BddVar.count(b)) {
		i++;
		n = i;
		BddVar[b] = n;
	} else {
		n = BddVar[b];	
	}
	return mgr->bddVar(i);
}

std::string PathTree::getNodeName(BasicBlock* b, bool start) {
	std::ostringstream name;
	if (start)
		name << "bs_" << b;
	else
		name << "b_" << b;
	return name.str();
}

void PathTree::DumpDotBDD(std::string filename) {
	std::ostringstream name;
	k++;
	name << filename << k << ".dot";

	int n = BddVar.size() + BddVarStart.size();

	char * inames[n];
	for (std::map<BasicBlock*,int>::iterator it = BddVar.begin(), et = BddVar.end(); it != et; it++) {
		inames[it->second] = strdup(getNodeName(it->first,false).c_str());
	}
	for (std::map<BasicBlock*,int>::iterator it = BddVarStart.begin(), et = BddVarStart.end(); it != et; it++) {
		inames[it->second] = strdup(getNodeName(it->first,true).c_str());
	}

    char const* onames[] = {"B"};
    DdNode *Dds[] =         {Bdd.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
    int result = Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            (char**) inames, (char**) onames, fp);
	fclose(fp);
}

int rank_vector(std::vector<BasicBlock*> v, BasicBlock* b) {
	int i = 0;
	while (i < v.size() && v[i] != b) {
		i++;
	}
	if (i == v.size()) 
		return -1;
	else
		return i;
}

void PathTree::insert(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	pathnode * v;
	BasicBlock * current;
	int i;

	workingpath.assign(path.begin(), path.end());
	v = &start;
	
	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlockStart(current));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlockStart(current));
		f = f * block;
	}
	//DumpDotBDD("before_insert");
	Bdd = Bdd + f;
	//DumpDotBDD("after_insert");
}

void PathTree::remove(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	pathnode * v;
	BasicBlock * current;
	int i;

	workingpath.assign(path.begin(), path.end());
	v = &start;

	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlockStart(current));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current));
		f = f * block;
	}
	Bdd = Bdd * !f;
}

void PathTree::clear() {
	Bdd = mgr->bddZero();
}

bool PathTree::exist(std::list<BasicBlock*> path) {
	std::list<BasicBlock*> workingpath;
	pathnode * v;
	BasicBlock * current;
	int i;
	
	workingpath.assign(path.begin(), path.end());
	v = &start;

	current = workingpath.front();
	workingpath.pop_front();
	BDD f = BDD(getBDDfromBasicBlockStart(current));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		BDD block = BDD(getBDDfromBasicBlock(current));
		f = f * block;
	}
	if (f <= Bdd) {
		return true;
	} else {
		return false;
	}
}
