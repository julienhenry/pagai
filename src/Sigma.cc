#include <sstream>

#include "cuddObj.hh"

#include "Analyzer.h"
#include "AIpass.h"
#include "Sigma.h"
#include "SMTpass.h"
#include "Debug.h"

//#DEFINE DUMP_ADD

Sigma::Sigma() {
	mgr = new Cudd(0,0);
	AddIndex=0;
	background = mgr->addZero().getNode();
	zero = mgr->addZero().getNode();
}

Sigma::~Sigma() {
	for (std::map<int,ADD*>::iterator it = Add.begin(), et = Add.end(); it != et; it++) {
		delete it->second;
	}
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

void Sigma::insert(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());
	
	current = workingpath.front();
	workingpath.pop_front();
	ADD f = ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	*Add[start] = *Add[start] + f;
}

void Sigma::remove(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	ADD f = ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}

	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	*Add[start] = *Add[start] * ~f;
}

void Sigma::clear() {
	for (std::map<int,ADD*>::iterator it = Add.begin(), et = Add.end(); it != et; it++) {
		*(it->second) = mgr->addZero();
	}
}

bool Sigma::exist(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	ADD f = ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	return (f <= *Add[start]);
}

bool Sigma::isZero(int start) {
	return !(*Add[start] > mgr->addZero());
}

void Sigma::setActualValue(std::list<BasicBlock*> path, int start, int value) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	
	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	ADD f = ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	// f corresponds to (start,path)
	for (int k = 0; k < value; k++) {
		*Add[start] += f;	
	}
}

int Sigma::getActualValue(std::list<BasicBlock*> path, int start) {
	std::list<BasicBlock*> workingpath;
	BasicBlock * current;
	

	workingpath.assign(path.begin(), path.end());

	current = workingpath.front();
	workingpath.pop_front();
	ADD f = ADD(getADDfromBasicBlock(current, AddVarSource));

	while (workingpath.size() > 0) {
		current = workingpath.front();
		workingpath.pop_front();
		ADD block = ADD(getADDfromBasicBlock(current, AddVar));
		f = f * block;
	}
	
	if (!Add.count(start))
		Add[start] = new ADD(mgr->addZero());
	// f corresponds to (start,path)
	ADD res = f * *Add[start];

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

int Sigma::getSigma(
		std::list<BasicBlock*> path, 
		int start,
		Abstract * Xtemp,
		params passID,
		bool source) {

	int res = getActualValue(path,start);
	if (res == -1) {

		if (path.front() == path.back())
			res = 2;
		else 
			res = 1;
		setActualValue(path,start,res);	
		DEBUG(
			AIPass::printPath(path);
			*Out << start << " is assigned to " << res-1 << "\n";
		);
	} else {
		//AIPass::printPath(path);
		//*Out << start << " is already assigned to " << res-1 << "\n";
	}
	
#ifdef DUMP_ADD
	std::ostringstream filename;
	filename << "ADD_" << path.front() << "_" << start;
	DumpDotADD(*Add[start],filename.str());
#endif
	return res -1;
}

void Sigma::DumpDotADD(ADD graph, std::string filename) {
	std::ostringstream name;
	name << filename << ".dot";

	int n = AddVar.size() + AddVarSource.size();
	char * inames[n];
	for (std::map<BasicBlock*,int>::iterator it = AddVar.begin(), et = AddVar.end(); it != et; it++) {
		inames[it->second] = strdup(SMTpass::getNodeName(it->first,false).c_str());
	}
	for (std::map<BasicBlock*,int>::iterator it = AddVarSource.begin(), et = AddVarSource.end(); it != et; it++) {
		inames[it->second] = strdup(SMTpass::getNodeName(it->first,true).c_str());
	}

    char const* onames[] = {"A"};
    DdNode *Dds[] = {graph.getNode()};
    int NumNodes = sizeof(onames)/sizeof(onames[0]);
    FILE* fp = fopen(name.str().c_str(), "w");
	Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            (char**) inames, (char**) onames, fp);
	fclose(fp);
}
