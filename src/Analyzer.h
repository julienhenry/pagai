#ifndef ANALYZER_H
#define ANALYZER_H

#include <set>
#include <map>

#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/CFG.h"


enum Apron_Manager_Type {
	BOX,
	OCT,
	PK,
	PKEQ,
	PPL_POLY,
	PPL_POLY_BAGNARA,
	PPL_GRID,
	PKGRID
};

enum Techniques {
	SIMPLE,
	LOOKAHEAD_WIDENING,
	GUIDED,
	PATH_FOCUSING,
	LW_WITH_PF,
	COMBINED_INCR,
	LW_WITH_PF_DISJ
};

enum SMTSolver {
	MATHSAT,
	Z3,
	Z3_QFNRA,
	SMTINTERPOL,
	CVC3, 
	API_Z3,
	API_YICES
};

std::string TechniquesToString(Techniques t);

std::string ApronManagerToString(Apron_Manager_Type D);

SMTSolver getSMTSolver();

Techniques getTechnique();

bool compareTechniques();

bool compareDomain();

bool compareNarrowing();

bool onlyOutputsRho();

bool useSourceName();

bool OutputAnnotatedFile();
char* getAnnotatedFilename();
char* getSourceFilename();

char* getFilename();

bool definedMain();
std::string getMain();

Apron_Manager_Type getApronManager();
Apron_Manager_Type getApronManager(int i);

bool useNewNarrowing();
bool useNewNarrowing(int i);

bool useThreshold();
bool useThreshold(int i);

extern llvm::raw_ostream *Out;

/// Functions ignored by Compare pass (because the analysis failed for
/// one technique)
extern std::set<llvm::Function*> ignoreFunction;


//extern std::map<Techniques,int> Passes;

#endif
