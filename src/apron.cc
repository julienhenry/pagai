#include <stdio.h>
#include <string>

#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"

#include "apron.h"
#include "Expr.h"

using namespace llvm;

ap_var_operations_t var_op_manager;

/// ap_var_to_string - new to_string function for the var_op_manager
///
char* ap_var_to_string(ap_var_t var) {
	Value * val = dyn_cast<Value>((Value*)var);
	std::string s_string;
	raw_string_ostream * s = new raw_string_ostream(s_string);

	if (val->hasName()) {
		*s << val->getName();
	} else {
		*s << *val;
	}
	
	std::string & name = s->str();
	size_t found;
	found=name.find_first_of("%");
	if (found!=std::string::npos) {
		name = name.substr(found);
		found=name.find_first_of(" ");
		if (found!=std::string::npos) {
			name.resize(found);
		}
	}
	char * cname = (char*)malloc((name.size()+1)*sizeof(char));
	strcpy(cname,name.c_str());
	delete s;
	return cname;
}

///
/// ap_var_compare - new compare function, working with Value * type
///
int ap_var_compare(ap_var_t v1, ap_var_t v2) {
	if (v1 == v2) return 0;
	if (v1 > v2) return 1;
	return -1;
}

///
/// ap_var_hash - hash function for ap_var_t
///
int ap_var_hash(ap_var_t v) {
	return 0;
}

// no copy, no free ! 
ap_var_t ap_var_copy(ap_var_t var) {return var;}
void ap_var_free(ap_var_t var) {}

///
/// init_apron - This function aims to change the functions for the apron var manager,
/// since var type is not char* but Value*.
///
void init_apron() {
	var_op_manager.compare = &ap_var_compare;
	var_op_manager.hash = &ap_var_hash;
	var_op_manager.copy = &ap_var_copy;
	var_op_manager.free = &ap_var_free;
	var_op_manager.to_string = &ap_var_to_string;

	ap_var_operations = &var_op_manager;
}

///
/// print_texpr - Print the apron expression in stdout 
///
void print_texpr(ap_texpr1_t * exp) {
	printf("Apron expression:\n");
	ap_texpr1_fprint(stdout,exp);
	printf("\n");
	fflush(stdout);
}
