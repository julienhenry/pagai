#ifndef ANALYZER_H
#define ANALYZER_H

enum SMT_Solver {
	Z3_MANAGER,
	YICES_MANAGER
};

SMT_Solver getSMTSolver();

#endif
