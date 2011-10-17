#include "Debug.h"
#include "Analyzer.h"

#include <sys/time.h>

int n_paths;
int n_iterations;
int n_totalpaths;

struct timeval SMT_time;
struct timeval Total_time;


struct timeval Now() { 
	struct timeval tp; 
	gettimeofday(&tp,NULL); 
	return tp;
	//return double(tp.tv_sec) + double(tp.tv_usec)*1e-6; 
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
