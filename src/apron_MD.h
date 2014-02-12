/**
 * \file apron_MD.h
 * \brief Declares functions related to the Apron interface
 * \author Julien Henry
 */
#ifndef _APRON_MD_H 
#define _APRON_MD_H 

#include "llvm/Support/CFG.h"

#include "ap_global1.h"

#include "Analyzer.h"
#include "Node.h"

using namespace llvm;

void ap_tcons1_t_to_MDNode(ap_tcons1_t & cons, llvm::Instruction * Inst, std::vector<llvm::Value*> * met);

void coeff_to_MDNode(ap_coeff_t * a, llvm::Instruction * Inst, std::vector<llvm::Value*> * met);
void ap_texpr1_t_to_MDNode(ap_texpr1_t & expr, llvm::Instruction * Inst, std::vector<llvm::Value*> * met);

void texpr0_to_MDNode(ap_texpr0_t* a, ap_environment_t * env, llvm::Instruction * Inst, std::vector<llvm::Value*> * met);
void texpr0_node_to_MDNode(ap_texpr0_node_t * a, ap_environment_t * env, llvm::Instruction * Inst, std::vector<llvm::Value*> * met);

#endif

