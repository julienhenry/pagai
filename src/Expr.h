#ifndef _HASHTABLES_H
#define _HASHTABLES_H

#include <map>

#include "llvm/BasicBlock.h"
#include "llvm/Value.h"

#include "ap_global1.h"

using namespace llvm;

extern std::map<Value *,ap_texpr1_t *> Exprs;

class Expr {
public:
	static ap_texpr1_t * get_ap_expr(Value * val);

	static ap_texpr1_t * create_ap_expr(Constant * val);

	static void set_ap_expr(Value * val, ap_texpr1_t * exp);

	static ap_texpr_rtype_t get_ap_type(Value * val);
};
#endif
