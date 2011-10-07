#include "cuddObj.hh"

#include "Analyzer.h"
#include "AIpass.h"
#include "Sigma.h"


Sigma::Sigma() {
	mgr = new Cudd(0,0);
	//mgr->makeVerbose();
	Add = new ADD(mgr->addZero());
	AddIndex=0;
	background = mgr->addZero().getNode();
	zero = mgr->addZero().getNode();
}

Sigma::~Sigma() {
	delete Add;
	delete mgr;	
}

ADD Sigma::getADDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map) {
	int n;
	if (!map.count(b)) {
		n = AddIndex;
		AddIndex++;
		map[b] = n;
	} else {
		n = map[b];	
	}
	return mgr->addVar(n);
}

ADD Sigma::getADDfromStartIndex(int start) {
	int n;
	if (!AddVarIndex.count(start)) {
		n = AddIndex;
		AddIndex++;
		AddVarIndex[start] = n;
	} else {
		n = AddVarIndex[start];	
	}
	return mgr->addVar(n);
}

void Sigma::insert(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	ADD f = ADD(getADDfromStartIndex(start));

	workingpath.assign(path.begin(), path.end());
	
	current = workingpath.front();
	workingpath.pop_front();
	f *= ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	
	*Add = *Add + f;
}

void Sigma::remove(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	ADD f = ADD(getADDfromStartIndex(start));

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	f *= ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}

	*Add = *Add * ~f;
}

void Sigma::clear() {
	*Add = mgr->addZero();
}

bool Sigma::exist(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	
	ADD f = ADD(getADDfromStartIndex(start));

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	f *= ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	return (f <= *Add);
}

bool Sigma::isZero() {
	return !(*Add > mgr->addZero());
}

void Sigma::setActualValue(std::list<BasicBlock*> path, int start, int value) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	
	ADD f = ADD(getADDfromStartIndex(start));

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	f *= ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	
	// f corresponds to (start,path)
	for (int k = 0; k < value; k++) {
		*Add += f;	
	}
}

int Sigma::getActualValue(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	
	ADD f = ADD(getADDfromStartIndex(start));

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	f *= ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	
	// f corresponds to (start,path)
	ADD res = f * *Add;

	DdNode *node;
	DdGen *gen;
	DdNode *N;
	
	for (gen = Cudd_FirstNode (mgr->getManager(), res.getNode(), &node);
	!Cudd_IsGenEmpty(gen);
	(void) Cudd_NextNode(gen,&node)) {
		N = Cudd_Regular(node);

		if (Cudd_IsConstant(N)) {
			if (node != zero) {
				int R = Cudd_V(N);
				return R;
			}
		}
	}
	// if we are here, it means the path does not exist in the diagram
	return -1;
}

int Sigma::getSigma(std::list<BasicBlock*> path, int start) {

	int res = getActualValue(path,start);
	if (res == -1) {

		if (path.front() == path.back())
			res = 2;
		else 
			res = 1;
		setActualValue(path,start,res);	
		AIPass::printPath(path);
		*Out << start << " is assigned to " << res-1 << "\n";
	} else {
		//AIPass::printPath(path);
		//*Out << start << " is already assigned to " << res-1 << "\n";
	}

	return res -1;
}
