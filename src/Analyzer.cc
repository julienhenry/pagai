#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>

#include "llvm/Support/Debug.h"

#include "Execute.h" 
#include "Analyzer.h" 

llvm::raw_ostream *Out;

void show_help() {
        std::cout << "Usage :\n \
\tanalyzer -h OR analyzer [-d] [-y]  -i <filename> \n \
-h : help\n \
-i : input file\n \
-d : debug\n";
}

int main(int argc, char* argv[]) {

	execute run;
    int o;
    bool help = false;
    bool bad_use = false;
    char* filename=NULL;
    char* outputname="";
	bool debug = false;


	 while ((o = getopt(argc, argv, "hdi:o:")) != -1) {
        switch (o) {
        case 'h':
            help = true;
            break;
        case 'd':
            debug = true;
            break;
        case 'i':
            filename = optarg;
            break;
        case 'o':
            outputname = optarg;
            break;
        case '?':
            std::cout << "Error : Unknown option" << optopt << "\n";
            bad_use = true;
        }   
    } 
    if (!help) {
        if (!filename) {
            std::cout << "No input file specified\n";
            bad_use = true;
        }

        if (bad_use) {
            std::cout << "Bad use\n";
            show_help();
            exit(EXIT_FAILURE);
        }
    } else {
        show_help();
        exit(EXIT_SUCCESS);
    }

	run.exec(filename,outputname);
	
	return 0;
}

