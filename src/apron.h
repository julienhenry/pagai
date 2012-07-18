#ifndef _APRON_H 
#define _APRON_H 

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


llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_tcons1_t & cons);

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_texpr1_t & cons);

llvm::raw_ostream& operator<<( llvm::raw_ostream &stream, ap_scalar_t & cons);


void texpr0_display(llvm::raw_ostream &stream, ap_texpr0_t* a, char ** name_of_dim);


int check_scalar(ap_scalar_t * a);
int check_coeff(ap_coeff_t * a);
int check_texpr0_node(ap_texpr0_node_t * a);
int check_texpr0(ap_texpr0_t * a);
#endif
