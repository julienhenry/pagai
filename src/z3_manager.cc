#include <algorithm>
#include <cstddef>
#include <string.h>
#include <sstream>

#include "llvm/Support/FormattedStream.h"

#include "z3_manager.h"

z3_manager::z3_manager() {
	Z3_config config = Z3_mk_config();
	Z3_set_param_value(config, "MODEL", "true");
	Z3_set_param_value(config, "MODEL_V2", "true");
	ctx = Z3_mk_context(config);
	Z3_set_logic(ctx,"QF_LIA");

	int_type = Z3_mk_int_sort(ctx);
	float_type = Z3_mk_real_sort(ctx);
	bool_type = Z3_mk_bool_sort(ctx);
	Z3_del_config(config);
}

z3_manager::~z3_manager() {
	Z3_del_context(ctx);
}

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
			return SMT_mk_true();
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
			return SMT_mk_true();
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


SMT_expr z3_manager::SMT_mk_real (double x) {
	std::ostringstream oss;
	oss << x;
	std::string val = oss.str();
	Z3_symbol symbol = Z3_mk_string_symbol(ctx,val.c_str());
	return Z3_mk_const(ctx,symbol,(Z3_sort)float_type);
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
	printf("%s",Z3_benchmark_to_smtlib_string(ctx,
				"name",
				"logic",
				"unknown",
				"",
				0,
				NULL,
				(Z3_ast)a));
}

bool z3_manager::SMT_check(SMT_expr a, std::set<std::string> * true_booleans){
	Z3_model m = NULL;
	Z3_assert_cnstr(ctx,(Z3_ast)a);
	Z3_lbool result = Z3_check_and_get_model(ctx, &m);

	switch (result) {
		case Z3_L_FALSE:
			printf("unsat\n");
			return false;
		case Z3_L_UNDEF:
			printf("Unknown");
			return false;
		case Z3_L_TRUE:
			printf("MODEL:\n");
			unsigned n = Z3_get_model_num_constants(ctx,m);
			for (unsigned i = 0; i < n; i++) {
				Z3_func_decl decl = Z3_get_model_constant(ctx,m,i);
				Z3_ast v;
				Z3_eval_func_decl (ctx,m,decl,&v);
				Z3_symbol symbol = Z3_get_decl_name(ctx,decl);
				std::string name (Z3_get_symbol_string (ctx,symbol));
				printf("%s ",name.c_str());

				Z3_sort_kind sort = Z3_get_sort_kind(ctx,Z3_get_sort(ctx,v));

				switch (sort) {
					case Z3_BOOL_SORT: 
						switch (Z3_get_bool_value(ctx,v)) {
							case Z3_L_FALSE:
								printf("false\n");
								break;
							case Z3_L_UNDEF:
								printf("undef\n");
								break;
							case Z3_L_TRUE:
								printf("true\n");
								true_booleans->insert(name);
								break;
						}
						break;
					case Z3_INT_SORT:
						int i;
						Z3_get_numeral_int (ctx,v,&i);
						printf("%d\n",i);
						break;
					case Z3_REAL_SORT:
						printf("real value\n");
						break;
					case Z3_BV_SORT:
						printf("bv value\n");
						break;
					case Z3_UNINTERPRETED_SORT:
						printf("uninterpreted value\n");
						break;
					default:
						printf("unknown sort\n");
						break;
				}
			}
			fflush(stdout);
			break;
	}
	fflush(stdout);
	return true;
}

void z3_manager::push_context() {
	Z3_push(ctx);
}

void z3_manager::pop_context() {
	Z3_pop(ctx, 1);
}
