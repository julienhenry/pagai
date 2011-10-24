#include "SMTpass.h"
#include "Abstract.h"
#include "AbstractGopan.h"
#include "Expr.h"

int Abstract::compare(Abstract * d) {
	SMTpass * LSMT = SMTpass::getInstance();
	bool f = false;
	bool g = false;

	LSMT->push_context();
	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,this);
	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,d);

	LSMT->push_context();
	// f = A and not B
	std::vector<SMT_expr> cunj;
	cunj.push_back(A_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		f = true;
	}
	LSMT->pop_context();

	// g = B and not A
	cunj.clear();
	cunj.push_back(B_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
	test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		g = true;
	}

	LSMT->pop_context();
	
	if (!f && !g) {
		// eq
		return 0;
	} else if (!f && g) {
		// lt
		return 1;
	} else if (f && !g) {
		// gt
		return -1;
	} else {
		// not comparable
		return -2;
	}
}

bool Abstract::CanJoinPrecisely(AbstractMan * aman, Abstract * A) {
	bool res = true;
	SMTpass * LSMT = SMTpass::getInstance();
	std::vector<Abstract*> Join;
	ap_environment_t * env = common_environment(main->env, A->main->env);
	Join.push_back(aman->NewAbstract(this));
	Join.push_back(aman->NewAbstract(A));
	Abstract * J = aman->NewAbstract(this);
	J->join_array(env,Join);

	LSMT->push_context();
	SMT_expr A_smt = LSMT->AbstractToSmt(NULL,this);
	SMT_expr B_smt = LSMT->AbstractToSmt(NULL,A);
	SMT_expr J_smt = LSMT->AbstractToSmt(NULL,J);
	
	std::vector<SMT_expr> cunj;
	cunj.push_back(J_smt);
	cunj.push_back(LSMT->man->SMT_mk_not(A_smt));
	cunj.push_back(LSMT->man->SMT_mk_not(B_smt));
	SMT_expr test = LSMT->man->SMT_mk_and(cunj);
	if (LSMT->SMTsolve_simple(test)) {
		res = false;
	}
	LSMT->pop_context();
	delete J;
	return res;
}

bool Abstract::is_leq(Abstract * d) {
	// in the case we compare two AbstractGopan, we have to slightly change the
	// comparison
	if (dynamic_cast<AbstractGopan*>(d) 
		&& dynamic_cast<AbstractGopan*>(this)) {
		if (ap_abstract1_is_eq(man,main,d->main)) {
			if (ap_abstract1_is_leq(man,pilot,d->pilot) || d->pilot == NULL) 
				return true; 
			else 
				return false;
		}
		return ap_abstract1_is_leq(man,main,d->main);
	}

	return (compare(d) >= 0);
}

bool Abstract::is_eq(Abstract * d) {
	return (compare(d) == 0);
}
