#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CFG.h"

#include "Pr.h"
#include "Analyzer.h"

using namespace llvm;

char Pr::ID = 0;
static RegisterPass<Pr>
X("Pr set", "Pr set computation pass", false, true);

const char * Pr::getPassName() const {
	return "Pr";
}
		
std::map<Function*,std::set<BasicBlock*>*> Pr::Pr_set;
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

std::set<BasicBlock*>* Pr::getAssert(Function &F) {
	return Assert_set[&F];
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

bool Pr::check_acyclic(Function &F, Node * n) {
	std::stack<Node*> * S = new std::stack<Node*>();
	int N = 1;
	check_acyclic_rec(F,n,N,S);
	delete S;
	return true;
}

bool Pr::check_acyclic_rec(Function &F, Node * n, int & N,std::stack<Node*> * S) {
	Node * nsucc;
	index[n]=N;
	lowlink[n]=N;
	N++;
	S->push(n);
	isInStack[n]=true;
	for (succ_iterator s = succ_begin(n->bb), E = succ_end(n->bb); s != E; ++s) {
		BasicBlock * succ = *s;
		nsucc = Nodes[succ];
		switch (index[nsucc]) {
			case 0:
				if (!Pr_set[&F]->count(nsucc->bb))
					check_acyclic_rec(F,nsucc,N,S);
				lowlink[n] = std::min(lowlink[n],lowlink[nsucc]);
				break;
			default:
				if (isInStack[nsucc]) {
					//*Out << *nsucc->bb << " is a backedge node !!\n";
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
	std::set<BasicBlock*> * FAssert = new std::set<BasicBlock*>();
	BasicBlock * b;
	LoopInfo * LI = &(getAnalysis<LoopInfo>(F));

	FPr->insert(F.begin());

	for (Function::iterator i = F.begin(), e = F.end(); i != e; ++i) {
		b = i;
		index[Nodes[b]] = 0;
		if (LI->isLoopHeader(b)) {
			FPr->insert(b);
		}

		for (BasicBlock::iterator it = b->begin(), et = b->end(); it != et; ++it) {
			if (isa<ReturnInst>(*it)) {
				FPr->insert(b);
			} else if (CallInst * c = dyn_cast<CallInst>((Instruction*)it)) {
				Function * cF = c->getCalledFunction();
				if (cF != NULL) {
					std::string fname = cF->getName();
					std::string assert_fail ("__assert_fail");
					if (fname.compare(assert_fail) == 0) {
						FPr->insert(b);
						FAssert->insert(b);
					}
				}
			}
		}

	}
	Pr_set[&F] = FPr;
	Assert_set[&F] = FAssert;


	for (std::set<BasicBlock*>::iterator it = FPr->begin(), et = FPr->end(); it != et; it++)
		check_acyclic(F,Nodes[*it]);
}

std::set<BasicBlock*> Pr::getPrPredecessors(BasicBlock * b) {
	return Pr_pred[b];
}

std::set<BasicBlock*> Pr::getPrSuccessors(BasicBlock * b) {
	return Pr_succ[b];
}

