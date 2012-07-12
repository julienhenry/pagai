#include <stdio.h>
#include <string>

#include "llvm/Support/CFG.h"
#include "llvm/Support/FormattedStream.h"

#include "ap_global1.h"
#include "box.h"
#include "oct.h"
#include "ap_ppl.h"
#include "ap_pkgrid.h"
#include "pk.h"
#include "pkeq.h"

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
	}
	found=name.find_first_of(" ");
	if (found!=std::string::npos) {
		name.resize(found);
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

ap_manager_t * create_manager(Apron_Manager_Type man) {
	ap_manager_t * ap_man;
	switch (man) {
	case BOX:
		return box_manager_alloc(); // Apron boxes
	case OCT: 
		return oct_manager_alloc(); // Octagons
	case PK: 
		return pk_manager_alloc(true); // NewPolka strict polyhedra
	case PKEQ: 
		return pkeq_manager_alloc(); // NewPolka linear equalities
	case PPL_POLY: 
		return ap_ppl_poly_manager_alloc(true); // PPL strict polyhedra
	case PPL_POLY_BAGNARA: 
		ap_man = ap_ppl_poly_manager_alloc(true); // PPL strict polyhedra
		ap_funopt_t funopt;
		ap_funopt_init(&funopt);
		funopt.algorithm = 1;
		ap_manager_set_funopt(ap_man,AP_FUNID_WIDENING,&funopt);
		return ap_man;
	case PPL_GRID: 
		return ap_ppl_grid_manager_alloc(); // PPL grids
	case PKGRID: 
		// Polka strict polyhedra + PPL grids
		return ap_pkgrid_manager_alloc(	pk_manager_alloc(true),
										ap_ppl_grid_manager_alloc()); 
	}
}

