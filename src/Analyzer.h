#ifndef ANALYZER_H
#define ANALYZER_H
#include "llvm/Support/FormattedStream.h"

enum SMT_Solver {
	Z3_MANAGER,
	YICES_MANAGER
};

SMT_Solver getSMTSolver();

bool useLookaheadWidening();

extern llvm::raw_ostream *Out;

#endif
