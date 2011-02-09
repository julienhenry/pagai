#include "ap_global1.h"
#include <stdio.h>
#include <iostream>
void print_texpr(ap_texpr1_t * exp) {
	printf("\n");
	ap_texpr1_print(exp);
	std::cout << "\n";
}
