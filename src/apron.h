#ifndef _AP_DEBUG_H 
#define _AP_DEBUG_H 

#include "llvm/Support/CFG.h"

#include "ap_global1.h"

#include "Node.h"

using namespace llvm;

/// init_apron - initialize the apron library. 
/// This function has to be called at the very beginning of the pass
void init_apron();

/// print_texpr - print an apron expression on standard output
void print_texpr(ap_texpr1_t * exp);

#endif
