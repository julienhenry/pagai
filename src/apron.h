#ifndef _AP_DEBUG_H 
#define _AP_DEBUG_H 

#include "llvm/Support/CFG.h"

#include "ap_global1.h"

using namespace llvm;

void init_apron();

void print_texpr(ap_texpr1_t * exp);

#endif
