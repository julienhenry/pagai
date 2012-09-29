#ifndef _ABSTRACTMAN_H
#define _ABSTRACTMAN_H

#include "ap_global1.h"
#include "Abstract.h"
#include "Environment.h"

/// class that creates Abstract objects
class AbstractMan {

	public:
	virtual Abstract * NewAbstract(ap_manager_t * man, Environment * env) = 0;
	virtual Abstract * NewAbstract(Abstract * A) = 0;
};

/// class that create Abstract objects of type AbstractClassic
class AbstractManClassic : public AbstractMan {
	
	public:
	Abstract * NewAbstract(ap_manager_t * man, Environment * env);
	Abstract * NewAbstract(Abstract * A);
};

/// class that create Abstract objects of type AbstractGopan
class AbstractManGopan : public AbstractMan {

	public:
	Abstract * NewAbstract(ap_manager_t * man, Environment * env);
	Abstract * NewAbstract(Abstract * A);
};

/// class that create Abstract objects of type AbstractDisj
class AbstractManDisj : public AbstractMan {

	public:
	Abstract * NewAbstract(ap_manager_t * man, Environment * env);
	Abstract * NewAbstract(Abstract * A);
};
#endif
