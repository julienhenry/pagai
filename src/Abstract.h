#ifndef _ABSTRACT_H
#define _ABSTRACT_H

#include "ap_global1.h"

class Node;

class Abstract {

	private:
		ap_manager_t * man;
	public:
		ap_abstract1_t * main;
		ap_abstract1_t * pilot;

	public:

		Abstract(ap_manager_t* _man);

		~Abstract();

		bool is_leq(Abstract * d);

		void widening(Node * n);
};
#endif
