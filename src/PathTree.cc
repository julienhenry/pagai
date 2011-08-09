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
		n = BddIndex;
		levels[n] = b;
		BddIndex++;
		map[b] = n;
	} else {
		n = map[b];	
	}
	return mgr->bddVar(n);
}

std::string PathTree::getNodeName(BasicBlock* b, bool src, SMT * smt) {
	if (smt != NULL) return smt->getNodeName(b,src);
	std::ostringstream name;
	if (src)
		name << "bs_";
	else
		name << "b_";
	name << b;
	return name.str();
}

std::string PathTree::getStringFromLevel(int i, SMT * smt) {
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
    int result = Cudd_DumpDot(mgr->getManager(), NumNodes, Dds, 
            (char**) inames, (char**) onames, fp);
	fclose(fp);
}


DdNode * background, * zero;

SMT_expr PathTree::generateSMTformula(
	SMT * smt)
{
	DdNode * node = Bdd.getNode();
    int	i;
	std::vector<int> list;
	std::vector<SMT_expr> disjunct;

    background = mgr->bddZero().getNode();
    zero = Cudd_Not(mgr->bddOne().getNode());
    for (i = 0; i < BddIndex; i++) {
		list.push_back(2);
	}
    generateSMTformulaAux(smt,node,list,disjunct);
    return smt->man->SMT_mk_or(disjunct);

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
			if (v == 0) {
				*Out << " ~"<< bb << " ";
			} else if (v == 1) {
				*Out << " "<< bb << " ";
			} 
	    }
		*Out << "\n";
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
	DumpDotBDD(Bdd,"Bdd");
	//Bdd.PrintMinterm();
	//
	*Out << "new BDD!\n";
	//
	{
		BDD nnode;
		DdGen *gen;
		for(gen = Bdd.FirstNode(&nnode);
		!Cudd_IsGenEmpty(gen);
		(void) mgr->NextNode(gen, &nnode)) {
			//nnode, 
			DdNode * node = nnode.getNode();
			if (Cudd_IsConstant(node)) {
				*Out << node->index << " " << Cudd_V(node) << "\n";
			} else {
				//if (Cudd_IsComplement(Cudd_E(node))) {
				//*Out << "Complement node "
				//<< (Cudd_E(node)) << " "
				//<< Cudd_V(Cudd_Regular(Cudd_E(node)))
				//<< "\n";
				//} else {
				*Out << node->index << " "
				<< Cudd_T(node)->index << " "
				<< Cudd_E(node) << " ";
				if (Cudd_IsComplement(Cudd_E(node))) {
					*Out << "complemented";
				}
				*Out << "\n";
				//}
			}
		}
		Cudd_GenFree(gen);
	}

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
