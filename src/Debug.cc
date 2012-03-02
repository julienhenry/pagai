#include "Debug.h"
#include "Analyzer.h"

#include <sys/time.h>

int n_paths;
int n_totalpaths;

struct timeval SMT_time;
std::map<params,std::map<Function*,struct timeval> > Total_time;

std::map<params,std::map<Function*,int> > asc_iterations;
std::map<params,std::map<Function*,int> > desc_iterations;

struct timeval Now() { 
	struct timeval tp; 
	gettimeofday(&tp,NULL); 
	return tp;
} 

struct timeval add(struct timeval t1, struct timeval t2) {
	struct timeval res;
	res.tv_sec = t1.tv_sec + t2.tv_sec;
	res.tv_usec = t1.tv_usec + t2.tv_usec;
	if (res.tv_usec >= 1000000) {
		res.tv_usec -= 1000000;
		res.tv_sec++;
	}
	return res;
}

struct timeval sub(struct timeval t1, struct timeval t2) {
	struct timeval res;
	res.tv_sec = t1.tv_sec - t2.tv_sec;
	if (t1.tv_usec < t2.tv_usec) {
		t1.tv_usec += 1000000;
		res.tv_sec--;
	}
	res.tv_usec = t1.tv_usec - t2.tv_usec;
	return res;
}
