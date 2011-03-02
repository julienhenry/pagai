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

ap_texpr1_t * get_phivar_first_expr(Value * val);
void set_phivar_first_expr(Value * val, ap_texpr1_t * exp);

/// set_ap_expr - associate an Apron expression to a Value, which will be
/// remembered for future uses.
void set_ap_expr(Value * val, ap_texpr1_t * exp);

/// common_environment - computes and returns the least common environment of
/// two environments.
ap_environment_t * common_environment(
		ap_environment_t * env1,
		ap_environment_t * env2);

/// common_environment - modifies the two expression by giving them the same
/// least common environment.
void common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2);

/// get_ap_type - returns the Apron type of the LLVM Value
ap_texpr_rtype_t get_ap_type(Value * val);
#endif
