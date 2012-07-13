#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CFG.h"

#include "Pr.h"
#include "Analyzer.h"
#include "Debug.h"

using namespace llvm;

char Pr::ID = 0;
static RegisterPass<Pr>
X("Pr set", "Pr set computation pass", false, true);

const char * Pr::getPassName() const {
	return "Pr";
}
		
std::map<Function*,std::set<BasicBlock*>*> Pr::Pr_set;
std::map<Function*,std::set<BasicBlock*>*> Pr::Pw_set;
std::map<Function*,std::set<BasicBlock*>*> Pr::Assert_set;
std::map<BasicBlock*,std::set<BasicBlock*> > Pr::Pr_succ;
std::map<BasicBlock*,std::set<BasicBlock*> > Pr::Pr_pred;

Pr::Pr() : ModulePass(ID) {
}

Pr::~Pr() {
}

void Pr::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<LoopInfo>();
	AU.setPreservesAll();
}

std::set<BasicBlock*>* Pr::getPr(Function &F) {
	return Pr_set[&F];
}

std::set<BasicBlock*>* Pr::getPw(Function &F) {
	return Pw_set[&F];
}

std::set<BasicBlock*>* Pr::getAssert(Function &F) {
	return Assert_set[&F];
}

bool Pr::inPw(BasicBlock * b) {
	return Pw_set[b->getParent()]->count(b);
}

bool Pr::runOnModule(Module &M) {
	Function * F;

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		F = mIt;
		if (F->isDeclaration()) continue;
		computePr(*F);
	}
	return 0;
}

bool Pr::check_acyclic(Function &F,std::set<BasicBlock*>* FPr) {
	std::set<BasicBlock*> start;
	start.insert(FPr->begin(),FPr->end());
	start.insert(&F.front());

	for (std::set<BasicBlock*>::iterator it = start.begin(), et = start.end(); it != et; it++) {
		Node * n = Nodes[*it];
		for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
			index[Nodes[i]] = 0;
			isInStack[Nodes[i]] = false;
		}
		std::stack<Node*> S;
		int N = 1;
		if (!check_acyclic_rec(F,n,N,&S,FPr)) return false;
	}
	return true;
}

bool Pr::check_acyclic_rec(Function &F, Node * n, int & N,std::stack<Node*> * S,std::set<BasicBlock*>* FPr) {
	Node * nsucc;
	index[n]=N;
	lowlink[n]=N;
	N++;
	S->push(n);
	isInStack[n]=true;
	for (succ_iterator s = succ_begin(n->bb), E = succ_end(n->bb); s != E; ++s) {
		BasicBlock * succ = *s;
		nsucc = Nodes[succ];
		if (FPr->count(nsucc->bb))
			continue;
		switch (index[nsucc]) {
			case 0:
				if (!check_acyclic_rec(F,nsucc,N,S,FPr)) return false;
				lowlink[n] = std::min(lowlink[n],lowlink[nsucc]);
				break;
			default:
				if (isInStack[nsucc]) {
					lowlink[n] = std::min(lowlink[n],index[nsucc]);
					return false;
				}
		}
	}
	if (lowlink == index) {
		do {
			nsucc = S->top();
			S->pop();
			isInStack[nsucc]=false;
		} while (nsucc != n);
	}
	return true;
}

bool Pr::computeLoopHeaders(Function &F,std::set<BasicBlock*>* FPr) {
	Node * n = Nodes[&F.front()];
	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		index[Nodes[i]] = 0;
	}
	std::set<Node*> S;
	std::set<Node*> Seen;

	computeLoopHeaders_rec(F,n,&Seen,&S,FPr);
	
	return true;
}

bool Pr::computeLoopHeaders_rec(Function &F, Node * n,std::set<Node*> * Seen, std::set<Node*> * S,std::set<BasicBlock*>* FPr) {
	Node * nsucc;
	Seen->insert(n);

	if (S->count(n)) {
		FPr->insert(n->bb);
		return true;
	}
		
	std::set<Node*> Set;
	Set.insert(S->begin(),S->end());
	Set.insert(n);

	for (succ_iterator s = succ_begin(n->bb), E = succ_end(n->bb); s != E; ++s) {
		BasicBlock * succ = *s;
		nsucc = Nodes[succ];
		if (FPr->count(nsucc->bb))
			continue;
		if (Seen->count(nsucc)) {
			if (S->count(nsucc)) {
				FPr->insert(nsucc->bb);
			}
			continue;
		}
		computeLoopHeaders_rec(F,nsucc,Seen,&Set,FPr);
	}	
	return true;
}

// computePr - computes the set Pr of BasicBlocks
// for the moment - Pr = Pw + blocks with a ret inst
void Pr::computePr(Function &F) {
	Node * n;

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		if (Nodes.count(i) == 0) {
			n = new Node(i);
			Nodes[i] = n;
		}
	}

	if (F.size() > 0) {
		// we find the Strongly Connected Components
		Node * front = Nodes[&(F.front())];
		front->computeSCC();
	}

	std::set<BasicBlock*> * FPr = new std::set<BasicBlock*>();
	std::set<BasicBlock*> * FW = new std::set<BasicBlock*>();
	std::set<BasicBlock*> * FAssert = new std::set<BasicBlock*>();
	BasicBlock * b;
	LoopInfo * LI = &(getAnalysis<LoopInfo>(F));


	computeLoopHeaders(F,FPr);
//	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
//		b = i;
//		if (LI->isLoopHeader(b)) {
//			FPr->insert(b);
//		}
//	}
	Pr_set[&F] = FPr;
	Assert_set[&F] = FAssert;

	//minimize_Pr(F);
	if (!check_acyclic(F,FPr)) {
		//*Out << "ERROR : GRAPH IS NOT ACYCLIC !\n";
	}
	
	FW->insert(FPr->begin(),FPr->end());
	Pw_set[&F] = FW;

	FPr->insert(F.begin());
	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		b = i;
		for (BasicBlock::iterator it = b->begin(), et = b->end(); it != et; ++it) {
			if (isa<ReturnInst>(*it) || isa<UnreachableInst>(*it)) {
				FPr->insert(b); 
			} else if (CallInst * c = dyn_cast<CallInst>((Instruction*)it)) {
				Function * cF = c->getCalledFunction();
				if (cF != NULL) {
					std::string fname = cF->getName();
					static const std::string assert_fail ("__assert_fail");
					static const std::string undefined_behavior_trap ("undefined_behavior_trap_handler");
					static const std::string gnat_rcheck ("__gnat_rcheck_");
					if (fname.compare(assert_fail) == 0
					    || fname.compare(undefined_behavior_trap) == 0
					    || fname.substr(0, gnat_rcheck.length()).compare(gnat_rcheck) == 0) {
						FPr->insert(b);
						FAssert->insert(b);
					}
				}
			}
		}

	}

	std::set<BasicBlock*>* Pr = getPr(F);
	DEBUG(
		*Out << "FINAL Pr SET:\n";
		for (std::set<BasicBlock*>::iterator it = Pr->begin(), et = Pr->end(); it != et; it++) 
			*Out << **it << "\n";
		
	);
}

void Pr::minimize_Pr(Function &F) {
	std::set<BasicBlock*>* FPr = getPr(F);

	for (std::set<BasicBlock*>::iterator it = FPr->begin(), et = FPr->end(); it != et; it++) {
		BasicBlock * b = *it;
		std::set<BasicBlock*> Pr;
		Pr.insert(FPr->begin(),FPr->end());
		Pr.erase(b);
		if (check_acyclic(F,&Pr)) {
			Pr_set[&F]->erase(b);
			minimize_Pr(F);
			return;
		} else {
			FPr->insert(b);
		}
	}

}

std::set<BasicBlock*> Pr::getPrPredecessors(BasicBlock * b) {
	return Pr_pred[b];
}

std::set<BasicBlock*> Pr::getPrSuccessors(BasicBlock * b) {
	return Pr_succ[b];
}

