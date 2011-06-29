#ifndef DEBUG_H
#define DEBUG_H

#ifdef PRINT_DEBUG
#define DEBUG(X) X
#else
#define DEBUG(X) do { } while (0);
#endif

#include <time.h>

extern int n_paths;
extern int n_iterations;
extern int n_totalpaths;

extern struct timeval SMT_time;
extern struct timeval Total_time;

struct timeval Now();
struct timeval add(struct timeval t1, struct timeval t2);
struct timeval sub(struct timeval t1, struct timeval t2);
#endif
