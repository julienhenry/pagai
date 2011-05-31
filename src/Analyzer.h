#ifndef ANALYZER_H
#define ANALYZER_H

#include <set>

#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"

enum SMT_Solver {
	Z3_MANAGER,
	YICES_MANAGER
};

SMT_Solver getSMTSolver();

bool useLookaheadWidening();

bool compareTechniques();

extern llvm::raw_ostream *Out;

extern std::set<llvm::Function*> ignoreFunction;

#endif
