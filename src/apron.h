#ifndef _AP_DEBUG_H 
#define _AP_DEBUG_H 

#include "llvm/Support/CFG.h"

#include "ap_global1.h"

#include "Analyzer.h"
#include "Node.h"

using namespace llvm;

/// init_apron - initialize the apron library. 
/// This function has to be called at the very beginning of the pass
void init_apron();

ap_manager_t * create_manager(Apron_Manager_Type man);

char* ap_var_to_string(ap_var_t var);
#endif
