#include <stdio.h>

#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "apron.h"
#include <cstring>
#include <string>
#include "string.h"
using namespace llvm;

ap_var_operations_t var_op_manager;

/*
 * new to_string function for the var_op_manager
 */
char* ap_var_to_string(ap_var_t var) {
	Value * val = (Value *) var;
	//fouts() << "val = " << *val << "\n"; 
	std::string s_string;
	raw_string_ostream * s = new raw_string_ostream(s_string);
	*s << val;
	std::string & name = s->str();
	char * cname = new char [name.size()+1];
	strcpy(cname,name.c_str());
	return cname;
}

/*
 * This function aims to change the to_string function for printing variables,
 * since their type is not char* but pointers.
 */
void init_apron() {
	var_op_manager.compare = ap_var_operations_default.compare;
	var_op_manager.hash = ap_var_operations_default.hash;
	var_op_manager.copy = ap_var_operations_default.copy;
	var_op_manager.free = ap_var_operations_default.free;
	var_op_manager.to_string = &ap_var_to_string;

	ap_var_operations = &var_op_manager;
}

void print_texpr(ap_texpr1_t * exp) {
	printf("Apron expression:\n");
	ap_texpr1_fprint(stdout,exp);
	printf("\n");
}
