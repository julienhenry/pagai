#ifndef _HASHTABLES_H
#define _HASHTABLES_H

#include <map>

#include "llvm/BasicBlock.h"
#include "llvm/Value.h"

#include "ap_global1.h"

#include "Node.h"

using namespace llvm;

extern std::map<Value *,ap_texpr1_t *> Exprs;

class Expr {
public:
	static ap_texpr1_t * get_ap_expr(Node * n, Value * val);

	static ap_texpr1_t * create_ap_expr(Node * n, Constant * val);

	static void set_ap_expr(Value * val, ap_texpr1_t * exp);

	static void common_environment(ap_texpr1_t ** exp1, ap_texpr1_t ** exp2);

	static ap_texpr_rtype_t get_ap_type(Value * val);
};
#endif
