#ifndef COMPARENARROWING_H
#define COMPARENARROWING_H

#include "Compare.h"

#include "Pr.h"
#include "ModulePassWrapper.h"
#include "SMTpass.h"
#include "AIpf.h"
#include "AIopt.h"
#include "AIGopan.h"
#include "AIClassic.h"
#include "AIdis.h"
#include "Debug.h"

using namespace llvm;

template<Techniques T>
class CompareNarrowing : public ModulePass {
	
	private:	
		SMTpass * LSMT;
	
		std::map<params, sys::TimeValue *> Time;
		std::map<params, sys::TimeValue *> Eq_Time;
		std::map<params, int> total_asc;
		std::map<params, int> total_desc;

	public:
		/// It is crucial for LLVM's pass manager that
		/// this ID is different (in address) from a class to another,
		/// but the template instantiation mechanism will make sure it
		/// is the case.
		static char ID;

		CompareNarrowing() : ModulePass(ID)
		{}

		~CompareNarrowing() {}
		
		void getAnalysisUsage(AnalysisUsage &AU) const;

		void AddTime(params P, Function * F);
		void AddEqTime(params P, Function * F);
		
		void printTime(params P);
		void printEqTime(params P);

		void ComputeIterations(params P, Function * F);
		
		void printIterations(params P);

		bool runOnModule(Module &M);
	
		const char * getPassName() const;
};

template<Techniques T>
char CompareNarrowing<T>::ID = 0;
		
template<Techniques T>
const char * CompareNarrowing<T>::getPassName() const {
	return "CompareNarrowing";
}

template<Techniques T>
void CompareNarrowing<T>::getAnalysisUsage(AnalysisUsage &AU) const {
	switch(T) {
		case LOOKAHEAD_WIDENING:
			AU.addRequired<ModulePassWrapper<AIGopan, 0> >();
			AU.addRequired<ModulePassWrapper<AIGopan, 1> >();
			break;
		case PATH_FOCUSING:
			AU.addRequired<ModulePassWrapper<AIpf, 0> >();
			AU.addRequired<ModulePassWrapper<AIpf, 1> >();
			break;
		case LW_WITH_PF:
			AU.addRequired<ModulePassWrapper<AIopt, 0> >();
			AU.addRequired<ModulePassWrapper<AIopt, 1> >();
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
void CompareNarrowing<T>::ComputeIterations(params P, Function * F) {
	
	if (total_asc.count(P)) {
		total_asc[P] = total_asc[P] + asc_iterations[P][F];
		total_desc[P] = total_desc[P] + desc_iterations[P][F];
	} else {
		total_asc[P] = asc_iterations[P][F];
		total_desc[P] = desc_iterations[P][F];
	}
}

template<Techniques T>
void CompareNarrowing<T>::printIterations(params P) {
	if (!total_asc.count(P)) {
		total_asc[P] = 0;
		total_desc[P] = 0;
	}
	*Out << total_asc[P] << " " << total_desc[P] << "\n";
}

template<Techniques T>
void CompareNarrowing<T>::AddEqTime(params P, Function * F) {
	
	if (Eq_Time.count(P)) {
		*Eq_Time[P] = *Eq_Time[P]+*Total_time[P][F];
	} else {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Eq_Time[P] = zero;
		*Eq_Time[P] = *Total_time[P][F];
	}
}

template<Techniques T>
void CompareNarrowing<T>::AddTime(params P, Function * F) {
	
	if (Time.count(P)) {
		*Time[P] = *Time[P]+*Total_time[P][F];
	} else {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[P] = zero;
		*Time[P] = *Total_time[P][F];
	}
}

template<Techniques T>
void CompareNarrowing<T>::printEqTime(params P) {
	if (!Eq_Time.count(P)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Eq_Time[P] = zero;
	}
	*Out 
		<< " " << Eq_Time[P]->seconds() 
		<< " " << Eq_Time[P]->microseconds() 
		<<  "\n";
}

template<Techniques T>
void CompareNarrowing<T>::printTime(params P) {
	if (!Time.count(P)) {
		sys::TimeValue * zero = new sys::TimeValue((double)0);
		Time[P] = zero;
	}
	*Out 
		<< " " << Time[P]->seconds() 
		<< " " << Time[P]->microseconds() 
		<<  "\n";
}

template<Techniques T>
bool CompareNarrowing<T>::runOnModule(Module &M) {
	Function * F;
	BasicBlock * b;
	Node * n;
	LSMT = SMTpass::getInstance();

	int gt = 0;
	int lt = 0;
	int eq = 0;
	int un = 0;

	int F_equal = 0;
	int F_distinct = 0;

	params P1, P2;
	P1.T = T;
	P2.T = T;
	P1.D = getApronManager(0);
	P2.D = getApronManager(1);
	P1.N = useNewNarrowing(0);
	P2.N = useNewNarrowing(1);
	P1.TH = useThreshold(0);
	P2.TH = useThreshold(1);

	Out->changeColor(raw_ostream::BLUE,true);
	*Out << "\n\n\n"
			<< "---------------------------------\n"
			<< "-      COMPARING NARROWING      -\n"
			<< "---------------------------------\n";
	Out->resetColor();

	for (Module::iterator mIt = M.begin() ; mIt != M.end() ; ++mIt) {
		LSMT->reset_SMTcontext();
		F = mIt;
		
		// if the function is only a declaration, do nothing
		if (F->begin() == F->end()) continue;

		if (ignoreFunction.count(F) > 0) continue;
		
		AddTime(P1,F);
		AddTime(P2,F);
		ComputeIterations(P1,F);
		ComputeIterations(P2,F);

		bool distinct = false;

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i) {
			b = i;
			n = Nodes[b];
			if (Pr::getPw(*b->getParent())->count(b)) {

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
						distinct = true;
						lt++;
						break;
					case -1:
						distinct = true;
						gt++;
						break;
					case -2:
						distinct = true;
						un++;
						break;
					default:
						distinct = true;
						break;
				}
			}
		}

		if (!distinct) {
			AddEqTime(P1,F);
			AddEqTime(P2,F);
			F_equal++;
		} else {
			F_distinct++;
		}
	}

	Out->changeColor(raw_ostream::MAGENTA,true);
	*Out << ApronManagerToString(getApronManager(0)) << " ABSTRACT DOMAIN\n\n\n" 
		<< "IMPROVED NARROWING -- CLASSIC" << "\n";
	Out->resetColor();
	*Out << "\nTIME\n";
	printTime(P1);
	printTime(P2);
	
	printEqTime(P1);
	printEqTime(P2);

	*Out << "\nFUNCTIONS\n";
	*Out 
		<< F_equal 
		<< " " << F_distinct 
		<<  " " << F_equal+F_distinct << "\n";

	*Out << "\nITERATIONS\n";
	printIterations(P1);
	printIterations(P2);

	*Out << "\n\n";
	*Out << "EQ " << eq << "\n";
	*Out << "LT " << lt << "\n";
	*Out << "GT " << gt << "\n";
	*Out << "UN " << un << "\n";

	*Out << "\n\nMATRIX:\n";
	*Out << eq << " " << lt << " " << gt << " " << un << "\n";
	return true;
}
#endif
