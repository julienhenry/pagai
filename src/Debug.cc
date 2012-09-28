#include "Debug.h"
#include "Analyzer.h"

#include <sys/time.h>

int n_paths;
int n_totalpaths;

std::map<params,std::map<Function*,sys::TimeValue *> > Total_time;

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

//struct timeval Now() { 
//	struct timeval tp; 
//	gettimeofday(&tp,NULL); 
//	return tp;
//} 
//
//struct timeval add(struct timeval t1, struct timeval t2) {
//	struct timeval res;
//	long int sec = 0;
//	long int usec = 0;
//	sec = t1.tv_sec + t2.tv_sec;
//	usec = t1.tv_usec + t2.tv_usec;
//	while (usec >= 1000000) {
//		usec -= 1000000;
//		sec++;
//	}
//	res.tv_sec = sec;
//	res.tv_usec = usec;
//	return res;
//}
//
//struct timeval sub(struct timeval t1, struct timeval t2) {
//	struct timeval res;
//	res.tv_sec = t1.tv_sec - t2.tv_sec;
//	if (t1.tv_usec < t2.tv_usec) {
//		t1.tv_usec += 1000000;
//		res.tv_sec--;
//	}
//	res.tv_usec = t1.tv_usec - t2.tv_usec;
//	return res;
//}
