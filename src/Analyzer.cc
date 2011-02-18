#include <string>
#include <iostream>

#include "Execute.h" 

char* check_args(int argc, char* argv[]) {

// NOT IMPLEMENTED
	
	return argv[1];

}

int main(int argc, char* argv[]) {

	char* filename;
	execute run;
	
	try {
		filename = check_args(argc, argv);
	} catch (int i) {
		std::cerr << "The program should be used with a parameter."
			  << "\n";
		return 1;
	}

	run.exec(filename,"");
	
	return 0;
}
