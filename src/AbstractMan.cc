#include "Analyzer.h"
#include "AbstractMan.h"
#include "Abstract.h"
#include "AbstractClassic.h"
#include "AbstractGopan.h"
#include "AbstractDisj.h"

Abstract * AbstractManClassic::NewAbstract(ap_manager_t * man, ap_environment_t * env) {
	return new AbstractClassic(man,env);
}

Abstract * AbstractManClassic::NewAbstract(Abstract * A) {
	return new AbstractClassic(A);
}

Abstract * AbstractManGopan::NewAbstract(ap_manager_t * man, ap_environment_t * env) {
	return new AbstractGopan(man,env);
}

Abstract * AbstractManGopan::NewAbstract(Abstract * A) {
	return new AbstractGopan(A);
}

Abstract * AbstractManDisj::NewAbstract(ap_manager_t * man, ap_environment_t * env) {
	return new AbstractDisj(man,env);
}

Abstract * AbstractManDisj::NewAbstract(Abstract * A) {
	return new AbstractDisj(A);
}
