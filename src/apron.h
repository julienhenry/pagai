#ifndef _AP_DEBUG_H 
#define _AP_DEBUG_H 

#include "llvm/Support/CFG.h"

#include "ap_global1.h"

#include "Node.h"

using namespace llvm;

void init_apron();

void print_texpr(ap_texpr1_t * exp);

bool abstract_inclusion (
		ap_manager_t * man,
		abstract *c,
		abstract *d
		);
#endif
