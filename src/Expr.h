#ifndef _EXPR_H
#define _EXPR_H

#include <map>

#include "llvm/BasicBlock.h"
#include "llvm/Value.h"

#include "ap_global1.h"

#include "Node.h"

using namespace llvm;

/// get_ap_expr - returns the expression associated to a specific value in a
/// Node.
ap_texpr1_t * get_ap_expr(Node * n, Value * val);

ap_texpr1_t * get_phivar_previous_expr(Value * val);
void set_phivar_previous_expr(Value * val, ap_texpr1_t * exp);

/// set_ap_expr - associate an Apron expression to a Value, which will be
/// remembered for future uses.
void set_ap_expr(Value * val, ap_texpr1_t * exp);

/// common_environment - computes and returns the least common environment of
/// two environments.
ap_environment_t * common_environment(
		ap_environment_t * env1,
		ap_environment_t * env2);

/// intersect_environment - compute the intersection of the two environments.
ap_environment_t * intersect_environment(
		ap_environment_t * env1,
		ap_environment_t * env2);

/// common_environment - modifies the two expression by giving them the same
/// least common environment.
void common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2);

/// get_ap_type - compute the Apron type of the LLVM Value
/// return 0 iff the type is int or real, 1 in the other cases
int get_ap_type(Value * val,ap_texpr_rtype_t &ap_type);

void environment_print(ap_environment_t * env);
void texpr1_print(ap_texpr1_t * expr);
void tcons1_array_print(ap_tcons1_array_t * cons);
#endif
