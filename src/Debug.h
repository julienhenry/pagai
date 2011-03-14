#ifndef DEBUG_H
#define DEBUG_H

#ifdef PRINT_DEBUG
#define DEBUG(X) X
#else
#define DEBUG(X) do { } while (0);
#endif

#endif
