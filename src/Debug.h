/**
 * \file Debug.h
 * \brief Declares some Debug functions and variables
 * \author Julien Henry
 */
#ifndef DEBUG_H
#define DEBUG_H

// Don't use LLVM's DEBUG statement
#undef DEBUG

//#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#define DEBUG(X) do {X} while (0)
#else
#define DEBUG(X) do { } while (0)
#endif

#ifdef PRINT_DEBUG_SMT
#define DEBUG_SMT(X) do {X} while (0)
#else
#define DEBUG_SMT(X) do { } while (0)
#endif

#ifdef HAS_Z3 
#define Z3(X)  X
#else
#define Z3(X)  
#endif


#include <time.h>
#include <map>
#include "Node.h"

#include "llvm/Support/TimeValue.h"

using namespace llvm;

extern int n_paths;
extern int n_totalpaths;

extern std::map<params,std::map<Function*,sys::TimeValue*> > Total_time;
extern std::map<params,std::map<Function*,sys::TimeValue*> > Total_time_SMT;

/**
 * \brief count the number of ascending iterations
 */
extern std::map<params,std::map<Function*,int> > asc_iterations;

/** 
 * \brief count the number of descending iterations
 */
extern std::map<params,std::map<Function*,int> > desc_iterations;

extern void ReleaseTimingData();

/**
 * \brief Functions ignored by Compare pass (because the analysis failed for
 * one technique)
 */
extern std::map<params,std::set<llvm::Function*> > ignoreFunction;
extern std::map<llvm::Function*,int> numNarrowingSeedsInFunction;

extern bool ignored(Function * F);
extern int nb_ignored();

#endif
