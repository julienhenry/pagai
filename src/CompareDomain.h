#ifndef COMPAREDOMAIN_H
#define COMPAREDOMAIN_H

#include "Compare.h"

#include "Pr.h"
#include "ModulePassWrapper.h"
#include "SMTpass.h"
#include "AIpf.h"
#include "AIpf_incr.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "Debug.h"

using namespace llvm;

template<Techniques T>
class CompareDomain : public ModulePass {
	
	private:	
		SMTpass * LSMT;
	
	public:
		/// It is crucial for LLVM's pass manager that
		/// this ID is different (in address) from a class to another,
		/// but the template instantiation mechanism will make sure it
		/// is the case.
		static char ID;

		CompareDomain() : ModulePass(ID)
		{}

		~CompareDomain() {}
		
		void getAnalysisUsage(AnalysisUsage &AU) const;

		const char * getPassName() const;

		bool runOnModule(Module &M);
};

template<Techniques T>
char CompareDomain<T>::ID = 0;
		
template<Techniques T>
const char * CompareDomain<T>::getPassName() const {
	return "CompareDomain";
}

template<Techniques T>
void CompareDomain<T>::getAnalysisUsage(AnalysisUsage &AU) const {
	switch(T) {
		case LOOKAHEAD_WIDENING:
			AU.addRequired<ModulePassWrapper<AIGopan, 0> >();
			AU.addRequired<ModulePassWrapper<AIGopan, 1> >();
			break;
		case PATH_FOCUSING:
			AU.addRequired<ModulePassWrapper<AIpf, 0> >();
			AU.addRequired<ModulePassWrapper<AIpf, 1> >();
			break;
		case PATH_FOCUSING_INCR:
			AU.addRequired<ModulePassWrapper<AIpf_incr, 0> >();
			AU.addRequired<ModulePassWrapper<AIpf_incr, 1> >();
			break;
		case LW_WITH_PF:
			AU.addRequired<ModulePassWrapper<AIopt, 0> >();
			AU.addRequired<ModulePassWrapper<AIopt, 1> >();
			break;
		case COMBINED_INCR:
			AU.addRequired<ModulePassWrapper<AIopt_incr, 0> >();
			AU.addRequired<ModulePassWrapper<AIopt_incr, 1> >();
			break;
		case SIMPLE:
			AU.addRequired<ModulePassWrapper<AIClassic, 0> >();
			AU.addRequired<ModulePassWrapper<AIClassic, 1> >();
			break;
		case LW_WITH_PF_DISJ:
			AU.addRequired<ModulePassWrapper<AIdis, 0> >();
			AU.addRequired<ModulePassWrapper<AIdis, 1> >();
			break;
	}
	AU.addRequired<Pr>();
	AU.setPreservesAll();
}


template<Techniques T>
bool CompareDomain<T>::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = SMTpass::getInstance();

	int gt = 0;
	int lt = 0;
	int eq = 0;
	int un = 0;

	Out->changeColor(raw_ostream::BLUE,true);
	*Out << "\n\n\n"
			<< "---------------------------------\n"
			<< "-   COMPARING ABSTRACT DOMAINS  -\n"
			<< "---------------------------------\n";
	Out->resetColor();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		LSMT->reset_SMTcontext();
		F = mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;

		if (ignoreFunction.count(F) > 0) continue;
		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (Pr::getPw(*b->getParent())->count(b)) {
				// TODO
				params P1, P2;
				P1.T = T;
				P2.T = T;
				P1.D = getApronManager(0);
				P2.D = getApronManager(1);
				P1.N = useNewNarrowing(0);
				P2.N = useNewNarrowing(1);
				P1.TH = useThreshold(0);
				P2.TH = useThreshold(1);
				DEBUG(
				*Out << "Comparing the two abstracts :\n";
				n->X_s[P1]->print();
				n->X_s[P2]->print();
				);
				switch (n->X_s[P1]->compare(n->X_s[P2])) {
					case 0:
						eq++;
						break;
					case 1:
						lt++;
						break;
					case -1:
						gt++;
						break;
					case -2:
						un++;
						break;
					default:
						break;
				}
			}
		}
	}

	Out->changeColor(raw_ostream::MAGENTA,true);
	*Out << ApronManagerToString(getApronManager(0)) << " - " 
		<< ApronManagerToString(getApronManager(1)) << "\n";
	Out->resetColor();
	*Out << "\n";
	*Out << "EQ " << eq << "\n";
	*Out << "LT " << lt << "\n";
	*Out << "GT " << gt << "\n";
	*Out << "UN " << un << "\n";

	*Out << "\n\nMATRIX:\n";
	*Out << eq << " " << lt << " " << gt << " " << un << "\n";
	return true;
}
#endif
