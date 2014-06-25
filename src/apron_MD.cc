/**
 * \file apron.cc
 * \brief Implementation of Apron interface
 * \author Julien Henry
 */
#include <stdio.h>
#include <string>

#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"
#include "box.h"
#include "oct.h"
#include "pk.h"
#include "pkeq.h"

#include "apron.h"
#include "Expr.h"
#include "SMTpass.h"
#include "Analyzer.h"
#include "Info.h"
#include "recoverName.h"
#include "apron_MD.h"

using namespace llvm;

void ap_tcons1_t_to_MDNode(ap_tcons1_t & cons, llvm::Instruction * Inst, std::vector<llvm::Value*> * met) {
	ap_constyp_t* constyp = ap_tcons1_constypref(&cons);
	ap_texpr1_t texpr = ap_tcons1_texpr1ref(&cons);
	ap_scalar_t* scalar;

	LLVMContext& C = Inst->getContext();
	std::vector<llvm::Value*> MD_texpr;
	ap_texpr1_t_to_MDNode(texpr,Inst,&MD_texpr);
	met->push_back(MDNode::get(C,MD_texpr));
	switch (*constyp) {
		case AP_CONS_EQ:
			met->push_back(MDString::get(C, "=="));
			break;
		case AP_CONS_SUPEQ:
			met->push_back(MDString::get(C, ">="));
			break;
		case AP_CONS_SUP:
			met->push_back(MDString::get(C, ">"));
			break;
		case AP_CONS_EQMOD:
			//scalar = ap_tcons1_scalarref(&cons);
			//stream << " = 0 MOD " << *scalar;
			//met->push_back(MDString::get(C, "=0 MOD "));
			assert(false && "not implemented");
			break;
		case AP_CONS_DISEQ:
			met->push_back(MDString::get(C, "!="));
			break;
	}
	met->push_back(MDString::get(C, "0"));
	//ConstantInt * i = ConstantInt::get(llvm::Type::getInt32Ty(C),0);
	//met->push_back(i);
}

void coeff_to_MDNode(ap_coeff_t * a, llvm::Instruction * Inst, std::vector<llvm::Value*> * met) {
	switch(a->discr){
		case AP_COEFF_SCALAR:
			{
			std::string s_string;
			raw_string_ostream * s = new raw_string_ostream(s_string);
			*s << *(a->val.scalar);
			LLVMContext& C = Inst->getContext();
			met->push_back(MDString::get(C, s->str()));
			}
		 	break;
		case AP_COEFF_INTERVAL:
			assert(false && "not implemented");
	}
}

void ap_texpr1_t_to_MDNode(ap_texpr1_t & expr, llvm::Instruction * Inst, std::vector<llvm::Value*> * met) {
	texpr0_to_MDNode(expr.texpr0, expr.env,Inst,met);
}

void texpr0_to_MDNode(ap_texpr0_t* a, ap_environment_t * env, llvm::Instruction * Inst, std::vector<llvm::Value*> * met) {
	if (!a) return;
	LLVMContext& C = Inst->getContext();
#if 1
	switch (a->discr) {
		case AP_TEXPR_CST:
			coeff_to_MDNode(&a->val.cst,Inst,met);
			break;
		case AP_TEXPR_NODE:
			texpr0_node_to_MDNode(a->val.node, env,Inst,met);
			break;
		case AP_TEXPR_DIM:
			{
				ap_dim_t d = a->val.dim;
				ap_var_t var = ap_environment_var_of_dim(env,d);
				Value * val = (Value*)var;
				MDString * dim = MDString::get(C, ap_var_to_string(var));
				MDNode* N = MDNode::get(C,dim);
				if (Instruction * i = dyn_cast<Instruction>(val)) {
					i->setMetadata("pagai.var", N);
				}
			}
			break;
		default:
			break;
	}
#else
	switch (a->discr) {
		case AP_TEXPR_CST:
			coeff_to_MDNode(&a->val.cst,Inst,met);
			break;
		case AP_TEXPR_DIM:
			{
				ap_dim_t d = a->val.dim;
				ap_var_t var = ap_environment_var_of_dim(env,d);
				Value * val = (Value*)var;
				MDString * dim = MDString::get(C, ap_var_to_string(var));
				MDNode* N = MDNode::get(C,dim);
				if (Instruction * i = dyn_cast<Instruction>(val)) {
					met->push_back(N);
					//met->push_back(val);
					i->setMetadata("pagai.var", N);
				} else if (Argument * arg = dyn_cast<Argument>(val)) {
					met->push_back(N);
					//met->push_back(val);
				} else {
					assert(false);
				}
			}
			break;
		case AP_TEXPR_NODE:
			if (true) {
				std::vector<llvm::Value*> child;
				texpr0_node_to_MDNode(a->val.node,env,Inst,&child);
				MDNode* N = MDNode::get(C,child);
				met->push_back(N);
			} else {
				texpr0_node_to_MDNode(a->val.node, env,Inst,met);
			}
			break;
		default:
			assert(false);
	}
#endif
}

void texpr0_node_to_MDNode(ap_texpr0_node_t * a, ap_environment_t * env, llvm::Instruction * Inst, std::vector<llvm::Value*> * met) {
	int prec = ap_texpr_op_precedence[a->op];
	LLVMContext& C = Inst->getContext();

#if 1
	if (a->exprB) {
		/* left argument (if binary) */
		texpr0_to_MDNode(a->exprA, env,Inst,met);
	}
	/* right argument */
	ap_texpr0_t* arg = a->exprB ? a->exprB : a->exprA;
	texpr0_to_MDNode(arg,env,Inst,met);
#else
	if (a->exprB) {
		int A = check_texpr0(a->exprA);
		int B = check_texpr0(a->exprB);

		if ((a->op == 0 || a->op == 1)) {
			// this is a + or a -
			if (A == ZERO) {
				texpr0_to_MDNode(a->exprB,env,Inst,met);
				return;
			}
			if (B == ZERO) {
				texpr0_to_MDNode(a->exprA,env,Inst,met);
				return;
			}
		}

		if (a->op == 2) {
			// this is a *
			if (A == ZERO) {
				return;
			}
			if (B == ZERO) {
				return;
			}
			if (A == ONE) {
				texpr0_to_MDNode(a->exprB,env,Inst,met);
				return;
			}
			if (B == ONE) {
				texpr0_to_MDNode(a->exprA,env,Inst,met);
				return;
			}
		}

		/* left argument (if binary) */
		int prec2 = ap_texpr0_precedence(a->exprA);
		texpr0_to_MDNode(a->exprA, env,Inst,met);
	}

	/* operator & rounding mode */
	std::string op = ap_texpr_op_name[a->op];
	met->push_back(MDString::get(C, op));

	/* right argument */
	{
		ap_texpr0_t* arg = a->exprB ? a->exprB : a->exprA;
		int prec2 = ap_texpr0_precedence(arg);
		texpr0_to_MDNode(arg,env,Inst,met);
	}
#endif
}

