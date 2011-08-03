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
Apron_Manager_Type ap_manager;
llvm::raw_ostream *Out;

void show_help() {
        std::cout << "Usage :\n \
\tanalyzer -h OR analyzer [options]  -i <filename> \n \
-h : help\n \
-d : abstract domain\n \
         possible abstract domains:\n \
		   * box (Apron boxes)\n \
		   * oct (Octagons)\n \
		   * pk (NewPolka strict polyhedra)\n \
		   * pkeq (NewPolka linear equalities)\n \
		   * ppl_poly (PPL strict polyhedra)\n \
		   * ppl_grid (PPL grids)\n \
		   * pkgrid (Polka strict polyhedra + PPL grids)\n \
		example of option: -d box\n \
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

Apron_Manager_Type getApronManager() {
	return ap_manager;
}


bool setApronManager(char * domain) {
	std::string d;
	d.assign(domain);
	
	if (!d.compare("box")) {
		ap_manager = BOX;
	} else if (!d.compare("oct")) {
		ap_manager = OCT;
	} else if (!d.compare("pk")) {
		ap_manager = PK;
	} else if (!d.compare("pkeq")) {
		ap_manager = PKEQ;
	} else if (!d.compare("ppl_poly")) {
		ap_manager = PPL_POLY;
	} else if (!d.compare("ppl_grid")) {
		ap_manager = PPL_GRID;
	} else if (!d.compare("pkgrid")) {
		ap_manager = PKGRID;
	} else {
		std::cout << "Wrong parameter defining the abstract domain\n";
		return 1;
	}
	return 0;
}

std::set<llvm::Function*> ignoreFunction;

int main(int argc, char* argv[]) {

	execute run;
    int o;
    bool help = false;
    bool bad_use = false;
    char* filename=NULL;
    char* outputname="";
	char* domain;
	bool debug = false;

	manager = Z3_MANAGER;
	ap_manager = PK;
	gopan = false;
	compare = false;
	n_totalpaths = 0;
	n_paths = 0;
	n_iterations = 0;
	SMT_time.tv_sec = 0;
	SMT_time.tv_usec = 0;
	Total_time = Now();

	 while ((o = getopt(argc, argv, "hDd:i:o:ycg")) != -1) {
        switch (o) {
        case 'h':
            help = true;
            break;
        case 'D':
            debug = true;
            break;
        case 'c':
            compare = true;
            break;
        case 'g':
            gopan = true;
            break;
        case 'd':
            domain = optarg;
			if (setApronManager(domain)) {
				bad_use = true;
			}
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

