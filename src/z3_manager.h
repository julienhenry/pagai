#ifndef Z3_H
#define Z3_H

#include "SMT_manager.h"

class z3 {

	public:
		SMT_expr SMT_mk_true();
		SMT_expr SMT_mk_false();

		SMT_var SMT_mk_bool_var(char * name);

		SMT_expr mk_expr_from_bool_var(SMT_var var);

		SMT_expr mk_or (SMT_expr args[], unsigned n);

		SMT_expr mk_and (SMT_expr args[], unsigned n);

		SMT_expr mk_eq (SMT_expr a1, SMT_expr a2);

		SMT_expr mk_diseq (SMT_expr a1, SMT_expr a2);

		SMT_expr mk_ite (SMT_expr c, SMT_expr t, SMT_expr e);

		SMT_expr mk_not (SMT_expr a);

};
#endif
