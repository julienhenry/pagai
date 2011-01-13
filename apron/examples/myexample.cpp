#include "ap_global1.h"
#include "pk.h"

#include <iostream>
#include <string>

using namespace std;

void ex1(ap_manager_t* man) {

	int x;

	printf("******************************\n");
	printf("ex1\n");
	printf("Library %s, version %s\n",man->library,man->version);
	printf("******************************\n");

	ap_var_t dimensions[4];
	dimensions[0]= (void*) "x";
	dimensions[1]= (void*) "y";
	dimensions[2]= (void*) "z";
	dimensions[3]= (void*) "w";
	ap_var_t dimensions2[3] = {(void*)"x",(void*)"y",(void*)"z"};

	x = ap_var_operations_default.hash(dimensions);
	printf("hash(dimensions) = %d\n",x);

	x = ap_var_operations_default.compare(dimensions,dimensions2);
	printf("compare(dimensions,dimensions2) = %d\n",x);

	string s = string(ap_var_operations_default.to_string(dimensions));
	cout << "dimensions to string = " <<  s << endl;


	// creating environments

	ap_environment_t * env = ap_environment_alloc(dimensions,4,NULL,0);
	ap_environment_fdump(stdout,env);


	/* =================================================================== */
	/* Creation of polyhedra 
	   1/2x+2/3y=1, [1,2]<=z+2w<=4 */
	/* =================================================================== */

	/* 0. Create the array */
	ap_lincons1_array_t array = ap_lincons1_array_make(env,3);

	/* 1.a Creation of an equality constraint 1/2x+2/3y=1 */
	ap_linexpr1_t expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
	ap_lincons1_t cons = ap_lincons1_make(AP_CONS_EQ,&expr,NULL);
	/* Now expr is memory-managed by cons */ 

	/* 1.b Fill the constraint */ 
	ap_lincons1_set_list(&cons,
			AP_COEFF_S_FRAC,1,2,"x",
			AP_COEFF_S_FRAC,2,3,"y",
			AP_CST_S_INT,-1,
			AP_END);
	/* 1.c Put in the array */
	ap_lincons1_array_set(&array,0,&cons);
	/* Now cons is memory-managed by array */ 

	/* 2.a Creation of an inequality constraint [1,2]<=z+2w */
	expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
	cons = ap_lincons1_make(AP_CONS_SUPEQ,&expr,NULL);
	/* The old cons is not lost, because it is stored in the array.
	   It would be an error to clear it (same for expr). */
	/* 2.b Fill the constraint */ 
	ap_lincons1_set_list(&cons,
			AP_COEFF_S_INT,1,"z",
			AP_COEFF_S_DOUBLE,2.0,"w",
			AP_CST_I_INT,-2,-1,
			AP_END);
	/* 2.c Put in the array */
	ap_lincons1_array_set(&array,1,&cons);

	/* 3.a Creation of an equality constraint 0 <= -z -2w + 4*/
	expr = ap_linexpr1_make(env,AP_LINEXPR_SPARSE,2);
	cons = ap_lincons1_make(AP_CONS_SUPEQ,&expr,NULL);

	/* 3.b Fill the constraints */
	ap_lincons1_set_list(&cons,
			AP_COEFF_S_INT,-1,"z",
			AP_COEFF_S_INT,-2,"w",
			AP_CST_S_INT,4,
			AP_END);

	/* 3.c Put in the array */
	ap_lincons1_array_set(&array,2,&cons);

	/* 4. Creation of an abstract value */
	ap_abstract1_t abs = ap_abstract1_of_lincons_array(man,env,&array);

	fprintf(stdout,"Abstract value:\n");
	ap_abstract1_fprint(stdout,man,&abs);

	/* deallocation */
	ap_lincons1_array_clear(&array);


	ap_environment_free(env);
}


int main() {	

	ap_manager_t* man;

	man = pk_manager_alloc(true);
	ex1(man);
	ap_manager_free(man);

	return 1;
}
