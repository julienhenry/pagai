#ifndef ANALYZER_H
#define ANALYZER_H

#include <set>

#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"

enum SMT_Solver {
	Z3_MANAGER,
	YICES_MANAGER
};

enum Apron_Manager_Type {
	BOX,
	OCT,
	PK,
	PKEQ,
	PPL_POLY,
	PPL_GRID,
	PKGRID
};

SMT_Solver getSMTSolver();

bool useLookaheadWidening();

bool compareTechniques();

bool onlyOutputsRho();

char* getFilename();

Apron_Manager_Type getApronManager();

extern llvm::raw_ostream *Out;

extern std::set<llvm::Function*> ignoreFunction;

#endif
