#ifndef DEBUG_H
#define DEBUG_H

#ifdef PRINT_DEBUG
#define DEBUG(X) X
#else
#define DEBUG(X) do { } while (0);
#endif

extern int n_paths;
extern int n_iterations;
extern int n_totalpaths;

#endif
