#ifndef _ABSTRACTMAN_H
#define _ABSTRACTMAN_H

#include "ap_global1.h"
#include "Abstract.h"

class AbstractMan {

	public:
	virtual Abstract * NewAbstract(ap_manager_t * man, ap_environment_t * env) = 0;
	virtual Abstract * NewAbstract(Abstract * A) = 0;
};


class AbstractManClassic : public AbstractMan {
	
	public:
	Abstract * NewAbstract(ap_manager_t * man, ap_environment_t * env);
	Abstract * NewAbstract(Abstract * A);
};

class AbstractManGopan : public AbstractMan {

	public:
	Abstract * NewAbstract(ap_manager_t * man, ap_environment_t * env);
	Abstract * NewAbstract(Abstract * A);
};

#endif
