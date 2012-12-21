/**
 * \file Debug.cc
 * \brief Implementation of some Debug utilities
 * \author Julien Henry
 */
#include "Debug.h"
#include "Analyzer.h"

#include <sys/time.h>

int n_paths;
int n_totalpaths;

std::map<params,std::map<Function*,sys::TimeValue *> > Total_time;
std::map<params,std::map<Function*,sys::TimeValue *> > Total_time_SMT;

std::map<params,std::map<Function*,int> > asc_iterations;
std::map<params,std::map<Function*,int> > desc_iterations;

void ReleaseTimingData() {
	std::map<params,std::map<Function*,sys::TimeValue *> >::iterator it = Total_time.begin(), et = Total_time.end();
	for (; it!=et; it++) {
		std::map<Function*,sys::TimeValue*> * m = &(*it).second;
		std::map<Function*,sys::TimeValue*>::iterator I = m->begin(), E = m->end();
		for (; I!=E; I++) {
			delete (*I).second;
		}
	}
}

std::map<params,std::set<llvm::Function*> > ignoreFunction;

bool ignored(Function * F) {
	std::map<params,std::set<llvm::Function*> >::iterator 
		it = ignoreFunction.begin(), 
		et = ignoreFunction.end();
	for (;it != et; it++) {
		if (it->second.count(F)) return true;
	}
	return false;
}

int nb_ignored() {
	std::set<llvm::Function*> ignored_funcs;
	std::map<params,std::set<llvm::Function*> >::iterator 
		it = ignoreFunction.begin(), 
		et = ignoreFunction.end();
	for (;it != et; it++) {
		ignored_funcs.insert(it->second.begin(),it->second.end());
	}
	return ignored_funcs.size();
}
