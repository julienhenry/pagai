#include <string>
#include <iostream>

#include "execute.h" 

char* check_args(int argc, char* argv[]) {

// NOT IMPLEMENTED
	
	if (argc != 2) {
		throw 1;
	}
	
	return argv[1];

}

int main(int argc, char* argv[]) {

	char* filename;
	execute run;
	
	try {
		filename = check_args(argc, argv);
	} catch (int i) {
		std::cerr << "The program should be used with a parameter."
			  << std::endl;
		return 1;
	}

	run.exec(filename,"out.smt");
	
	return 0;
}
