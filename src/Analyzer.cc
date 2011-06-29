#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>

#include "llvm/Support/Debug.h"

#include "Execute.h" 
#include "Analyzer.h" 
#include "Debug.h" 

SMT_Solver manager;
bool gopan;
bool compare;
llvm::raw_ostream *Out;

void show_help() {
        std::cout << "Usage :\n \
\tanalyzer -h OR analyzer [-g] [-y]  -i <filename> \n \
-h : help\n \
-i : input file\n \
-o : output file\n \
-g : use Lookahead Widening technique\n \
-c : compare the two techniques\n \
-y : use Yices instead of the Z3 SMT-solver (unused when -g)\n";
}

SMT_Solver getSMTSolver() {
	return manager;
}

bool useLookaheadWidening() {
	return gopan;
}

bool compareTechniques() {
	return compare;
}

std::set<llvm::Function*> ignoreFunction;

int main(int argc, char* argv[]) {

	execute run;
    int o;
    bool help = false;
    bool bad_use = false;
    char* filename=NULL;
    char* outputname="";
	bool debug = false;

	manager = Z3_MANAGER;
	gopan = false;
	compare = false;
	n_totalpaths = 0;
	n_paths = 0;
	n_iterations = 0;
	SMT_time.tv_sec = 0;
	SMT_time.tv_usec = 0;
	Total_time = Now();

	 while ((o = getopt(argc, argv, "hdi:o:ycg")) != -1) {
        switch (o) {
        case 'h':
            help = true;
            break;
        case 'd':
            debug = true;
            break;
        case 'c':
            compare = true;
            break;
        case 'g':
            gopan = true;
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

