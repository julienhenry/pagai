#include <algorithm>
#include <cstddef>
#include <string.h>

#include "llvm/Support/FormattedStream.h"

#include "z3_manager.h"

SMT_expr z3_manager::z3_manager::SMT_mk_true(){
	return Z3_mk_true(ctx);
}

SMT_expr z3_manager::SMT_mk_false(){
	return Z3_mk_false(ctx);
}


SMT_var z3_manager::SMT_mk_bool_var(std::string name){
	if (!vars.count(name)) {
		char * cstr = new char [name.size()+1];
		strcpy (cstr, name.c_str());
		vars[name] = Z3_mk_string_symbol(ctx,cstr);
		types[vars[name]] = bool_type;
	}
	return vars[name];
}

SMT_var z3_manager::SMT_mk_var(std::string name,SMT_type type){
	if (!vars.count(name)) {
		char * cstr = new char [name.size()+1];
		strcpy (cstr, name.c_str());
		vars[name] = Z3_mk_string_symbol(ctx,cstr);
		types[vars[name]] = type;
	}
	return vars[name];
}

SMT_expr z3_manager::SMT_mk_expr_from_bool_var(SMT_var var){
	return Z3_mk_const(ctx,(Z3_symbol)var,bool_type);
}

SMT_expr z3_manager::SMT_mk_expr_from_var(SMT_var var){
	return Z3_mk_const(ctx,(Z3_symbol)var,(Z3_sort)types[var]);
}

SMT_expr z3_manager::SMT_mk_or (std::vector<SMT_expr> args){
	std::vector<Z3_ast> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		arguments.push_back((Z3_ast)*b);
	}
	switch (arguments.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return Z3_mk_or(ctx, arguments.size(), &arguments[0]);
	}
}

SMT_expr z3_manager::SMT_mk_and (std::vector<SMT_expr> args){
	std::vector<Z3_ast> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		arguments.push_back((Z3_ast)*b);
	}
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return Z3_mk_and(ctx, arguments.size(), &arguments[0]);
	}
}

SMT_expr z3_manager::SMT_mk_eq (SMT_expr a1, SMT_expr a2){
	return Z3_mk_eq(ctx,(Z3_ast)a1,(Z3_ast)a2);
}

SMT_expr z3_manager::SMT_mk_diseq (SMT_expr a1, SMT_expr a2){
	Z3_ast args[2] = { (Z3_ast)a1, (Z3_ast)a2 };
	return Z3_mk_distinct(ctx, 2, args);
}

SMT_expr z3_manager::SMT_mk_ite (SMT_expr c, SMT_expr t, SMT_expr e){
	return Z3_mk_ite(ctx,(Z3_ast)c,(Z3_ast)t,(Z3_ast)e);
}

SMT_expr z3_manager::SMT_mk_not (SMT_expr a){
	return Z3_mk_not(ctx,(Z3_ast)a);
}

SMT_expr z3_manager::SMT_mk_num (int n){
	return Z3_mk_int(ctx, n, (Z3_sort)int_type);
}

SMT_expr z3_manager::SMT_mk_sum (std::vector<SMT_expr> args){
	std::vector<Z3_ast> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		arguments.push_back((Z3_ast)*b);
	}
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return Z3_mk_add(ctx, arguments.size(), &arguments[0]);
	}
}

SMT_expr z3_manager::SMT_mk_sub (std::vector<SMT_expr> args){
	std::vector<Z3_ast> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		arguments.push_back((Z3_ast)*b);
	}
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return Z3_mk_sub(ctx, arguments.size(), &arguments[0]);
	}
}

SMT_expr z3_manager::SMT_mk_mul (std::vector<SMT_expr> args){
	std::vector<Z3_ast> arguments;
	std::vector<SMT_expr>::iterator b = args.begin(), e = args.end();
	for (; b != e; ++b) {
		arguments.push_back((Z3_ast)*b);
	}
	switch (args.size()) {
		case 0:
			return NULL;
			break;
		case 1:
			return args[0];
			break;
		default:
			return Z3_mk_mul(ctx, arguments.size(), &arguments[0]);
	}
}

SMT_expr z3_manager::SMT_mk_lt (SMT_expr a1, SMT_expr a2){
	return Z3_mk_lt(ctx,(Z3_ast)a1,(Z3_ast)a2);
}

SMT_expr z3_manager::SMT_mk_le (SMT_expr a1, SMT_expr a2){
	return Z3_mk_le(ctx,(Z3_ast)a1,(Z3_ast)a2);
}

SMT_expr z3_manager::SMT_mk_gt (SMT_expr a1, SMT_expr a2){
	return Z3_mk_gt(ctx,(Z3_ast)a1,(Z3_ast)a2);
}

SMT_expr z3_manager::SMT_mk_ge (SMT_expr a1, SMT_expr a2){
	return Z3_mk_ge(ctx,(Z3_ast)a1,(Z3_ast)a2);
}


void z3_manager::SMT_print(SMT_expr a){
	Z3_model m = NULL;
	Z3_assert_cnstr(ctx,(Z3_ast)a);
	Z3_lbool result = Z3_check_and_get_model(ctx, &m);

printf("%s",Z3_benchmark_to_smtlib_string(ctx,
		"name",
		"logic",
		"status",
		"attributes",
		0,
		NULL,
		(Z3_ast)a));

	switch (result) {
		case Z3_L_FALSE:
			printf("unsat\n");
			break;
		case Z3_L_UNDEF:
			printf("Unknown\nPotential Model\n%s",Z3_model_to_string(ctx,m));
			break;
		case Z3_L_TRUE:
			std::string res (Z3_model_to_string(ctx,m));
			printf("number of elements in the model : %u\n",Z3_get_model_num_constants(ctx,m));
			printf("sat\nModel: %s \n",Z3_model_to_string(ctx,m));
			//printf("sat\nModel: %d \n",res.size());
			fflush(stdout);
			break;
	}
	if (m) {
		printf("Model is not NULL\n");
		Z3_del_model(ctx, m);
	} else {
		printf("Model is NULL\n");
	}
	fflush(stdout);
}

