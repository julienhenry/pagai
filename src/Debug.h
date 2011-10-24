#ifndef DEBUG_H
#define DEBUG_H

// Don't use LLVM's DEBUG statement
#undef DEBUG

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

#include <time.h>
#include <map>
#include "Node.h"

extern int n_paths;
extern int n_iterations;
extern int n_totalpaths;

extern struct timeval SMT_time;
extern std::map<params,std::map<Function*,struct timeval> > Total_time;

struct timeval Now();
struct timeval add(struct timeval t1, struct timeval t2);
struct timeval sub(struct timeval t1, struct timeval t2);
#endif
