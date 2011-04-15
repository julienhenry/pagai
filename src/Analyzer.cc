#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>

#include "llvm/Support/Debug.h"

#include "Execute.h" 
#include "Analyzer.h" 

SMT_Solver manager;
llvm::raw_ostream *Out;

void show_help() {
        puts("Usage :\n \
\tanalyzer -h OR analyzer [-d] [-y]  -i <filename> \n \
-h : help\n \
-i : input file\n \
-y : use Yices instead of Z3 SMT-solver\
-d : debug\n");
}

SMT_Solver getSMTSolver() {
	return manager;
}

int main(int argc, char* argv[]) {

	execute run;
    int o;
    bool help = false;
    bool bad_use = false;
    char* filename=NULL;
    char* outputname="";
	bool debug = false;

	manager = Z3_MANAGER;

	 while ((o = getopt(argc, argv, "hdi:o:y")) != -1) {
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
        case 'y':
            manager = YICES_MANAGER;
            break;
        case '?':
            printf("Error : Unknown option %c\n", optopt);
            bad_use = true;
        }   
    } 
    if (!help) {
        if (!filename) {
            printf("No input file specified.\n");
            bad_use = true;
        }

        if (bad_use) {
            printf("Bad use !\n");
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
