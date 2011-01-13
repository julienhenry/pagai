#include "ap_global1.h"
#include "pkeq.h"
#include "pk.h"
#include "ap_ppl.h"

#include <iostream>
#include <string>

using namespace std;

void widening_ex(ap_manager_t* man) {

	printf("******************************\n");
	printf("ex1\n");
	printf("Library %s, version %s\n",man->library,man->version);
	printf("******************************\n");


	ap_var_t dimensions[2] = {(void*)"x",(void*)"y"};
	
	/* creating environment */
	ap_environment_t * env = ap_environment_alloc(dimensions,2,NULL,0);
	ap_environment_fdump(stdout,env);

	/* =================================================================== */
	/* Creation of polyhedra 
	   x=0, y=0 */
	/* =================================================================== */

	ap_lincons1_array_t array = ap_lincons1_array_make(env,2);


	/* x=0 */
	ap_linexpr1_t expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
	ap_lincons1_t cons = ap_lincons1_make(AP_CONS_EQ,&expr,NULL);
		
	ap_lincons1_set_list(&cons,
			AP_COEFF_S_INT,1,"x",
			AP_END);
	ap_lincons1_array_set(&array,0,&cons);

	/* y=0 */
	expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
	cons = ap_lincons1_make(AP_CONS_EQ,&expr,NULL);
		
	ap_lincons1_set_list(&cons,
			AP_COEFF_S_INT,1,"y",
			AP_END);
	ap_lincons1_array_set(&array,1,&cons);
	
	/* Creation of an abstract value */
	ap_abstract1_t abs = ap_abstract1_of_lincons_array(man,env,&array);

	fprintf(stdout,"Abstract value:\n");
	ap_abstract1_fprint(stdout,man,&abs);

	ap_abstract1_t abs_next = ap_abstract1_copy(man,&abs);
	// we change x=0 into x=1
	expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
	ap_linexpr1_set_list(&expr,
		       AP_COEFF_S_INT,1,"x",
		       AP_CST_S_INT,1,
		       AP_END);
	abs_next = ap_abstract1_assign_linexpr(man,true,&abs_next,(ap_var_t)"x",&expr,NULL);

	ap_abstract1_fprint(stdout,man,&abs_next);

	// then we do the union
	abs_next = ap_abstract1_join(man,false,&abs,&abs_next);
	
	// and the widening
	ap_abstract1_t abs_widening = ap_abstract1_widening(man,&abs,&abs_next);
	cout << "After widening:" << endl;

	ap_abstract1_fprint(stdout,man,&abs_widening);

	/* deallocation */
	ap_lincons1_array_clear(&array);
	ap_environment_free(env);
}

int main() {	

	ap_manager_t* man;

	//man = ap_ppl_poly_manager_alloc(true);
	man = pk_manager_alloc(true);
	widening_ex(man);
	ap_manager_free(man);

	return 1;
}
