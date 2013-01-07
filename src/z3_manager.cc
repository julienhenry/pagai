/**
 * \file z3_manager.cc
 * \brief Implementation of the z3_manager class
 * \author Julien Henry
 */
#include <algorithm>
#include <cstddef>
#include <string.h>
#include <sstream>
#include <iostream>

#include "llvm/Support/FormattedStream.h"

#include <boost/thread/thread.hpp>

#include <gmp.h>
#include "z3_manager.h"
#include "Analyzer.h"
#include "Debug.h"

using namespace z3;

z3_manager::z3_manager() {
	config conf;
	conf.set("MODEL", "true");
	if (getTimeout() != 0) {
		std::ostringstream timeout;
		timeout << getTimeout()*1000;
		conf.set("SOFT_TIMEOUT", timeout.str().c_str());
	}
	ctx = new context(conf);


	int_type.i = new sort(ctx->int_sort());
	float_type.i = new sort(ctx->real_sort());
	bool_type.i = new sort(ctx->bool_sort());
	int0 = SMT_mk_num(0);
	real0 = SMT_mk_real(0.0);

	s = new solver(*ctx);
}

z3_manager::~z3_manager() {
	delete s;
	delete ctx;
}

SMT_expr z3_manager::SMT_mk_true(){
	SMT_expr res;
	res.i = new expr(ctx->bool_val(true));
	return res;
}

SMT_expr z3_manager::SMT_mk_false(){
	SMT_expr res;
	res.i = new expr(ctx->bool_val(false));
	return res;
}

SMT_var z3_manager::SMT_mk_bool_var(std::string name){
	if (!vars.count(name)) {
		char * cstr = new char [name.size()+1];
		strcpy (cstr, name.c_str());
		vars[name].i = new symbol(*ctx,Z3_mk_string_symbol(*ctx,cstr));
		delete [] cstr;
		types[vars[name]] = bool_type;
	}
	return vars[name];
}

SMT_var z3_manager::SMT_mk_var(std::string name, SMT_type type){
	if (!vars.count(name)) {
		char * cstr = new char [name.size()+1];
		strcpy (cstr, name.c_str());
		vars[name].i = new symbol(*ctx,Z3_mk_string_symbol(*ctx,cstr));
		delete [] cstr;
		types[vars[name]] = type;
	} 
	return vars[name];
}

SMT_expr z3_manager::SMT_mk_expr_from_bool_var(SMT_var var){
	SMT_expr res;
	res.i = new expr(ctx->constant(*(symbol*)var.i,ctx->bool_sort()));
	return res;
}

SMT_expr z3_manager::SMT_mk_expr_from_var(SMT_var var){
	SMT_expr res;
	res.i = new expr(ctx->constant(*(symbol*)var.i,*(sort*)types[var].i));
	return res;
}

SMT_expr z3_manager::SMT_mk_or (std::vector<SMT_expr> args){
	if (args.size() == 0) {
		return SMT_mk_true();
	}
	expr * e = new expr(*ctx);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	*e = *((expr*)(*B).i);
	B++;
	for (; B != E; ++B) {
		*e = *e || *((expr*)(*B).i);
	}
	SMT_expr res;
	res.i = new expr(*e);
	return res;
}

SMT_expr z3_manager::SMT_mk_and (std::vector<SMT_expr> args){
	if (args.size() == 0) {
		return SMT_mk_true();
	}
	expr * e = new expr(*ctx);
	*e = ctx->bool_val(true);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	*e = *((expr*)(*B).i);
	B++;
	for (; B != E; ++B) {
		*e = *e && *((expr*)(*B).i);
	}
	SMT_expr res;
	res.i = e;
	return res;
}

SMT_expr z3_manager::SMT_mk_eq (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i == *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_diseq (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i != *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e){
	SMT_expr res;
    expr ite  = to_expr(*ctx, Z3_mk_ite(*ctx, *(expr*)c.i, *(expr*)t.i, *(expr*)e.i));
	res.i = new expr(ite);
	return res;
}

SMT_expr z3_manager::SMT_mk_not (SMT_expr a){
	SMT_expr res;
	res.i = new expr( ! *(expr*)a.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_num (int n){
	SMT_expr res;
	res.i = new expr(ctx->int_val(n));;
	return res;
}

SMT_expr z3_manager::SMT_mk_num_mpq (mpq_t mpq) {
	SMT_expr res;
	char * x = mpq_get_str (NULL,10,mpq);
	res.i = new expr(ctx->int_val(x));;
	return res;
}

SMT_expr z3_manager::SMT_mk_real (double x) {
	SMT_expr res;
	mpq_t val;
	mpq_init(val);
	mpq_set_d(val,x);

	double den = mpz_get_d(mpq_denref(val));
	char * cnum = mpz_get_str(NULL,10,mpq_numref(val));
	char * cden = mpz_get_str(NULL,10,mpq_denref(val));

	std::ostringstream oss;
	if (den == 1)
		oss << cnum;
	else
		oss << cnum << "/" << cden;
	std::string r = oss.str();
	res.i = new expr(ctx->real_val(r.c_str()));;
	return res;
}

SMT_expr z3_manager::SMT_mk_sum (std::vector<SMT_expr> args){
	expr * e = new expr(*ctx);
	*e = ctx->int_val(0);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	for (; B != E; ++B) {
		*e = *e + *((expr*)(*B).i);
	}
	SMT_expr res;
	res.i = e;
	return res;
}

SMT_expr z3_manager::SMT_mk_sub (std::vector<SMT_expr> args){
	expr * e = new expr(*ctx);
	*e = ctx->int_val(0);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	for (; B != E; ++B) {
		*e = *e - *((expr*)(*B).i);
	}
	SMT_expr res;
	res.i = e;
	return res;
}

SMT_expr z3_manager::SMT_mk_mul (std::vector<SMT_expr> args){
	expr * e = new expr(*ctx);
	*e = ctx->int_val(1);
	std::vector<SMT_expr>::iterator B = args.begin(), E = args.end();
	for (; B != E; ++B) {
		*e = *e * *((expr*)(*B).i);
	}
	SMT_expr res;
	res.i = e;
	return res;
}

SMT_expr z3_manager::SMT_mk_sum (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i + *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_sub (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i - *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_mul (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i * *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_div (SMT_expr a1, SMT_expr a2, bool integer) {
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i / *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_rem (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	// TODO !!!
	res.i = new expr(*(expr*)a1.i / *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_xor (SMT_expr a1, SMT_expr a2) {
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i ^ *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i < *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i <= *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i > *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	SMT_expr res;
	res.i = new expr(*(expr*)a1.i >= *(expr*)a2.i);
	return res;
}

SMT_expr z3_manager::SMT_mk_int2real(SMT_expr a) {
	SMT_expr res;
    expr r  = to_real(*(expr*)a.i);
	res.i = new expr(r);
	return res;
}

SMT_expr z3_manager::SMT_mk_real2int(SMT_expr a) {
	SMT_expr res;
    expr r  = to_expr(*ctx, Z3_mk_real2int(*ctx, *(expr*)a.i));
	res.i = new expr(r);
	return res;
}

SMT_expr z3_manager::SMT_mk_is_int(SMT_expr a) {
	SMT_expr res;
	assert(Z3_get_sort_kind(*ctx, Z3_get_sort(*ctx, *(expr*)a.i)) == Z3_REAL_SORT);
    expr r  = to_expr(*ctx, Z3_mk_is_int(*ctx, *(expr*)a.i));
	res.i = new expr(r);
	return res;
}

SMT_expr z3_manager::SMT_mk_int0() {
	return int0;
}

SMT_expr z3_manager::SMT_mk_real0() {
	return real0;
}

void z3_manager::SMT_print(SMT_expr a){

	std::ostringstream oss;
	oss << "\"" << getFilename() << "\"";

	*Out << Z3_benchmark_to_smtlib_string(*ctx,
			oss.str().c_str(),
			"unknown",
			"unknown",
			"",
			0,
			NULL,
			*(expr*)a.i);
}

void z3_manager::SMT_assert(SMT_expr a){
	s->add(*(expr*)a.i);
}

int z3_manager::SMT_check(SMT_expr a, std::set<std::string> * true_booleans){
	int ret = 0;
	SMT_assert(a);
	//check_result result = s->check(1,(expr*)a.i);
	check_result result = s->check();
	switch (result) {
		case unsat:
			DEBUG(
					*Out << "unsat\n";
				 );
			ret = 0;
			break;
		case unknown:
			DEBUG(
			*Out << "unknown\n";
			);
			*Out << "UNKNOWN\n";
			ret = -1;
			break;
		case sat:
			DEBUG(
					*Out << "sat\n";
				 );
			ret = 1;
			DEBUG(
					DEBUG_SMT(
						*Out << Z3_model_to_string(ctx,m);
						);
				 );
			model m = s->get_model();
			unsigned n = m.num_consts();
			for (unsigned i = 0; i < n; i++) {
				func_decl decl = m.get_const_decl(i);
				expr v = m.get_const_interp(decl);
				std::string name = decl.name().str();

				if (v.is_bool()) {
						switch (Z3_get_bool_value(*ctx,v)) {
							case Z3_L_FALSE:
								//DEBUG(
								//	*Out << "false\n";
								//);
								break;
							case Z3_L_UNDEF:
								//DEBUG(
								//*Out << "undef\n";
								//);
								break;
							case Z3_L_TRUE:
								//DEBUG(
								//);
								true_booleans->insert(name);
								break;
						}

				}
			}
			break;
	}
	return ret;
}

void z3_manager::push_context() {
	s->push();
}

void z3_manager::pop_context() {
	s->pop();
}

bool z3_manager::interrupt() {
	Z3_interrupt(*ctx);
	return true;
}
