#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>

#include "llvm/Support/Debug.h"

#include "Execute.h" 
#include "Analyzer.h" 
#include "Debug.h" 

SMT_Solver manager;
Techniques technique;
bool compare;
bool onlyrho;
bool bagnara_widening;
Apron_Manager_Type ap_manager;
llvm::raw_ostream *Out;
char* filename;
int npass;
std::map<Techniques,int> Passes;

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
-t : use a specific technique\n \
		possible techniques:\n \
		  * lw (Lookahead Widening)\n \
		  * pf (Path Focusing)\n \
		  * lw+pf (combination of Lookahead Widening and Path Focusing)\n \
		  * s (simple abstract interpretation)\n \
-c : compare the 3 techniques (lw, pf and lw+pf)\n \
-b : use the BHRZ03 widening (Bagnara, Hill, Ricci & Zafanella, SAS’2003)\n \
-f : only outputs the SMT formula\n \
-y : use Yices instead of the Z3 SMT-solver (unused when -g)\n";
}

SMT_Solver getSMTSolver() {
	return manager;
}

Techniques getTechnique() {
	return technique;
}

bool compareTechniques() {
	return compare;
}

bool useBagnaraWidening() {
	return bagnara_widening;
}

bool onlyOutputsRho() {
	return onlyrho;
}

char* getFilename() {
	return filename;
}

Apron_Manager_Type getApronManager() {
	return ap_manager;
}

std::string TechniquesToString(Techniques t) {
	switch (t) {
		case LOOKAHEAD_WIDENING:
			return "LOOKAHEAD WIDENING";
		case PATH_FOCUSING: 
			return "PATH FOCUSING";
		case LW_WITH_PF:
			return "COMBINED TECHNIQUE";
		case SIMPLE:
			return "CLASSIC";
	}
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

bool setTechnique(char * t) {
	std::string d;
	d.assign(t);
	
	if (!d.compare("lw")) {
		technique = LOOKAHEAD_WIDENING;
	} else if (!d.compare("pf")) {
		technique = PATH_FOCUSING;
	} else if (!d.compare("lw+pf")) {
		technique = LW_WITH_PF;
	} else if (!d.compare("s")) {
		technique = SIMPLE;
	} else {
		std::cout << "Wrong parameter defining the technique you want to use\n";
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
    const char* outputname="";
	char* arg;
	bool debug = false;

	filename=NULL;
	manager = Z3_MANAGER;
	ap_manager = PK;
	technique = LW_WITH_PF;
	compare = false;
	onlyrho = false;
	bagnara_widening = false;
	n_totalpaths = 0;
	n_paths = 0;
	n_iterations = 0;
	SMT_time.tv_sec = 0;
	SMT_time.tv_usec = 0;
	Total_time = Now();
	npass = 0;

	static struct option long_options[] =
		{
			/* These options set a flag. */
			{"help", no_argument,       0, 'h'},
			{"debug",   no_argument,       0, 'D'},
			/* These options don't set a flag.
			   We distinguish them by their indices. */
			{"compare",     no_argument,       0, 'c'},
			{"technique",  required_argument,       0, 't'},
			{"domain",  required_argument, 0, 'd'},
			{"input",  required_argument, 0, 'i'},
			{"output",    required_argument, 0, 'o'},
			{"yices",    no_argument, 0, 'y'},
			{"printformula",    no_argument, 0, 'f'},
			{"bagnara",    no_argument, 0, 'b'},
			{0, 0, 0, 0}
		};
	/* getopt_long stores the option index here. */
	int option_index = 0;

	 while ((o = getopt_long(argc, argv, "hDd:i:o:ycft:b",long_options,&option_index)) != -1) {
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
        case 't':
            arg = optarg;
			if (setTechnique(arg)) {
				bad_use = true;
			}
            break;
        case 'd':
            arg = optarg;
			if (setApronManager(arg)) {
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
        case 'f':
            onlyrho = true;
            break;
        case 'b':
            bagnara_widening = true;
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

