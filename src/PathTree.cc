#include <map>
#include <sstream>

#include "cuddObj.hh"

#include "PathTree.h"
#include "Analyzer.h"

PathTree::PathTree() {
	mgr = new Cudd(0,0);
	//mgr->makeVerbose();
	Bdd = new BDD(mgr->bddZero());
	Bdd_prime = new BDD(mgr->bddZero());
	BddIndex=0;
}

PathTree::~PathTree() {
	delete mgr;	
}

BDD PathTree::getBDDfromBasicBlock(BasicBlock * b, std::map<BasicBlock*,int> &map) {
	int n;
	if (!map.count(b)) {
		n = BddIndex;
		levels[n] = b;
		BddIndex++;
		map[b] = n;
	} else {
		n = map[b];	
	}
	return mgr->bddVar(n);
}

const std::string PathTree::getNodeName(BasicBlock* b, bool src, SMT * smt) const {
	if (smt != NULL) return smt->getNodeName(b,src);
	std::ostringstream name;
	if (src)
		name << "bs_";
	else
		name << "b_";
	name << b;
	return name.str();
}

const std::string PathTree::getStringFromLevel(int const i, SMT * smt) {
	BasicBlock * bb = levels[i]; 
	if (BddVarStart.count(bb) && BddVarStart[bb]==i)
		return getNodeName(bb,true,smt);
	else
		return getNodeName(bb,false,smt);
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
	Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            (char**) inames, (char**) onames, fp);
	fclose(fp);
}

SMT_expr PathTree::generateSMTformula(
	SMT * smt)
{
	DdNode * node = Bdd->getNode();
    int	i;
	std::vector<int> list;
	std::vector<SMT_expr> disjunct;

    background = mgr->bddZero().getNode();
    zero = Cudd_Not(mgr->bddOne().getNode());
    for (i = 0; i < BddIndex; i++) {
		list.push_back(2);
	}
    generateSMTformulaAux(smt,node,list,disjunct);
	if (disjunct.size() == 0) {
		return smt->man->SMT_mk_false();
	} else {
		return smt->man->SMT_mk_or(disjunct);
	}
} 

void PathTree::generateSMTformulaAux(
	SMT * smt,
	DdNode * node /* current node */,
	std::vector<int> list, /* current recursion path */
	std::vector<SMT_expr> &disjunct)
{
    DdNode	*N,*Nv,*Nnv;
    int		i,v,index;

    N = Cudd_Regular(node);

    if (Cudd_IsConstant(N)) {
	/* Terminal case: Print one cube based on the current recursion
	** path, unless we have reached the background value (ADDs) or
	** the logical zero (BDDs).
	*/
	if (node != background && node != zero) {
		std::vector<SMT_expr> cunj;
	    for (i = 0; i < BddIndex; i++) {
			v = list[i];
			std::string bb = getStringFromLevel(i,smt);
			SMT_var bbvar = smt->man->SMT_mk_bool_var(bb);
			SMT_expr bbexpr = smt->man->SMT_mk_expr_from_bool_var(bbvar);

			switch (v) {
				case 0:
					bbexpr = smt->man->SMT_mk_not(bbexpr);
				case 1:
					cunj.push_back(bbexpr);
					break;
				default:
					break;
			}
	    }
		disjunct.push_back(smt->man->SMT_mk_and(cunj));
	}
    } else {
	Nv  = Cudd_T(N);
	Nnv = Cudd_E(N);
	if (Cudd_IsComplement(node)) {
	    Nv  = Cudd_Not(Nv);
	    Nnv = Cudd_Not(Nnv);
	}
	index = N->index;
	list[index] = 0;
	generateSMTformulaAux(smt,Nnv,list,disjunct);
	list[index] = 1;
	generateSMTformulaAux(smt,Nv,list,disjunct);
	list[index] = 2;
    }
    return;
} 


void PathTree::insert(std::list<BasicBlock*> path, bool primed) {
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
	if (primed) {
		*Bdd_prime = *Bdd_prime + f;
	} else {
		*Bdd = *Bdd + f;
	}
}

void PathTree::remove(std::list<BasicBlock*> path, bool primed) {
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

	if (primed) {
		*Bdd_prime = *Bdd_prime * !f;
	} else {
		*Bdd = *Bdd * !f;
	}
}

void PathTree::clear(bool primed) {
	if (primed) {
		*Bdd_prime = mgr->bddZero();
	} else {
		*Bdd = mgr->bddZero();
	}
}

bool PathTree::exist(std::list<BasicBlock*> path, bool primed) {
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
	bool res;
	if (primed) {
		res = f <= *Bdd_prime;
	} else {
		res = f <= *Bdd;
	}
	return res;
}

void PathTree::mergeBDD() {
	*Bdd = *Bdd + *Bdd_prime;
	*Bdd_prime = mgr->bddZero();
}

bool PathTree::isZero(bool primed) {
	if (primed) {
		return !(*Bdd_prime > mgr->bddZero());
	} else {
		return !(*Bdd > mgr->bddZero());
	}
}
