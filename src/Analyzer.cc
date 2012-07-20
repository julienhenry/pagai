#include <string>
#include <iostream>
#include <getopt.h>
#include <stdlib.h>

#include "Execute.h" 
#include "Analyzer.h" 
#include "Debug.h" 

SMTSolver Solver;
Techniques technique;
bool compare;
bool compare_Domain;
bool compare_Narrowing;
bool onlyrho;
bool bagnara_widening;
bool defined_main;
bool use_source_name;
bool output_annotated;
std::string main_function;
Apron_Manager_Type ap_manager[2];
bool Narrowing[2];
bool Threshold[2];
llvm::raw_ostream *Out;
char* filename;
char* annotatedFilename;
char* sourceFilename;
int npass;
std::map<Techniques,int> Passes;

void show_help() {
        std::cout << "Usage :\n \
\tpagai -h OR pagai [options]  -i <filename> \n \
--help (-h) : help\n \
--main <name> (-m) : only analyze the function <name>\n \
--source-name : output with the variable names from the source file instead of LLVM's names\n \
--domain (-d) : abstract domain\n \
         possible abstract domains:\n \
		   * box (Apron boxes)\n \
		   * oct (Octagons)\n \
		   * pk (NewPolka strict polyhedra), default\n \
		   * pkeq (NewPolka linear equalities)\n \
		   * ppl_poly (PPL strict polyhedra)\n \
		   * ppl_poly_bagnara (ppl_poly + widening from Bagnara & al, SAS’2003)\n \
		   * ppl_grid (PPL grids)\n \
		   * pkgrid (Polka strict polyhedra + PPL grids)\n \
		example: -d box\n \
--input (-i) : input file\n \
--technique (-t) : use a specific technique\n \
		possible techniques:\n \
		  * lw (Lookahead Widening, Gopan & Reps, SAS'06)\n \
		  * g (Guided Static Analysis, Gopan & Reps, SAS'07)\n \
		  * pf (Path Focusing, Monniaux & Gonnord, SAS'11)\n \
		  * lw+pf (Henry, Monniaux & Moy, SAS'12), default\n \
		  * s (simple abstract interpretation)\n \
		  * dis (lw+pf, using disjunctive invariants)\n \
		  * incr (s followed by lw+pf, results of s are injected in lw+pf)\n \
		example: -t pf\n \
--solver (-s) : select SMT Solver\n \
		  * z3 (default)\n \
		  * mathsat\n \
		  * smtinterpol\n \
		  * cvc3\n \
		  * z3_api (deprecated)\n \
		  * yices_api (deprecated)\n \
-n : new version of narrowing (only for s technique)\n \
-T : apply widening with threshold instead of classical widening\n \
-M : compare the two versions of narrowing (only for s technique)\n \
--compare (-c) : compare the 5 techniques (lw, pf, lw+pf, s and dis)\n \
--comparedomains (-C) : compare two abstract domains using the same technique\n \
          example: ./pagai -i <filename> -C --domain box --domain2 pkeq -t pf\n \
--printformula (-f) : only outputs the SMTpass formula\n \
";
}

SMTSolver getSMTSolver() {
	return Solver;
}

Techniques getTechnique() {
	return technique;
}

bool compareTechniques() {
	return compare;
}

bool compareDomain() {
	return compare_Domain;
}

bool compareNarrowing() {
	return compare_Narrowing;
}

bool onlyOutputsRho() {
	return onlyrho;
}

bool useSourceName() {
	return use_source_name;
}

bool OutputAnnotatedFile() {
	return output_annotated;
}

char* getAnnotatedFilename() {
	return annotatedFilename;
}

char* getSourceFilename() {
	return sourceFilename;
}

char* getFilename() {
	return filename;
}

Apron_Manager_Type getApronManager() {
	return ap_manager[0];
}

Apron_Manager_Type getApronManager(int i) {
	return ap_manager[i];
}

bool useNewNarrowing() {
	return Narrowing[0];
}

bool useNewNarrowing(int i) {
	return Narrowing[i];
}

bool useThreshold() {
	return Threshold[0];
}

bool useThreshold(int i) {
	return Threshold[i];
}

std::string TechniquesToString(Techniques t) {
	switch (t) {
		case LOOKAHEAD_WIDENING:
			return "LOOKAHEAD WIDENING";
		case PATH_FOCUSING: 
			return "PATH FOCUSING";
		case LW_WITH_PF:
			return "COMBINED";
		case COMBINED_INCR:
			return "COMBINED INCR";
		case SIMPLE:
			return "CLASSIC";
		case GUIDED:
			return "GUIDED";
		case LW_WITH_PF_DISJ:
			return "DISJUNCTIVE";
		default:
			abort();
	}
}

bool setApronManager(char * domain, int i) {
	std::string d;
	d.assign(domain);
	
	if (!d.compare("box")) {
		ap_manager[i] = BOX;
	} else if (!d.compare("oct")) {
		ap_manager[i] = OCT;
	} else if (!d.compare("pk")) {
		ap_manager[i] = PK;
	} else if (!d.compare("pkeq")) {
		ap_manager[i] = PKEQ;
	} else if (!d.compare("ppl_poly_bagnara")) {
		ap_manager[i] = PPL_POLY_BAGNARA;
	} else if (!d.compare("ppl_poly")) {
		ap_manager[i] = PPL_POLY;
	} else if (!d.compare("ppl_grid")) {
		ap_manager[i] = PPL_GRID;
	} else if (!d.compare("pkgrid")) {
		ap_manager[i] = PKGRID;
	} else {
		std::cout << "Wrong parameter defining the abstract domain\n";
		return 1;
	}
	return 0;
}

std::string ApronManagerToString(Apron_Manager_Type D) {
	switch (D) {
		case BOX:
			return "BOX";
		case OCT:
			return "OCT";
		case PK:
			return "PK";
		case PKEQ:
			return "PKEQ";
		case PPL_POLY:
			return "PPL_POLY";
		case PPL_POLY_BAGNARA:
			return "PPL_POLY_BAGNARA";
		case PPL_GRID:
			return "PPL_GRID";
		case PKGRID:
			return "PKGRID";
	}
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
	} else if (!d.compare("g")) {
		technique = GUIDED;
	} else if (!d.compare("dis")) {
		technique = LW_WITH_PF_DISJ;
	} else if (!d.compare("incr")) {
		technique = COMBINED_INCR;
	} else {
		std::cout << "Wrong parameter defining the technique you want to use\n";
		return 1;
	}
	return 0;
}

bool setSolver(char * t) {
	std::string d;
	d.assign(t);
	if (!d.compare("z3")) {
		Solver = Z3;
	} else if (!d.compare("mathsat")) {
		Solver = MATHSAT;
	} else if (!d.compare("smtinterpol")) {
		Solver = SMTINTERPOL;
	} else if (!d.compare("cvc3")) {
		Solver = CVC3;
	} else if (!d.compare("z3_api")) {
		Solver = API_Z3;
	} else if (!d.compare("yices_api")) {
		Solver = API_YICES;
	} else {
		std::cout << "Wrong parameter defining the solver\n";
		return 1;
	}
	return 0;
}

bool setMain(char * m) {
	main_function.assign(m);
	defined_main = true;
	return 0;
}

bool definedMain() {
	return defined_main;
}

std::string getMain() {
	return main_function;
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
	Solver = Z3;
	ap_manager[0] = PK;
	ap_manager[1] = PK;
	Narrowing[0] = false;
	Narrowing[1] = false;
	Threshold[0] = false;
	Threshold[1] = false;
	technique = LW_WITH_PF;
	compare = false;
	compare_Domain = false;
	compare_Narrowing = false;
	onlyrho = false;
	defined_main = false;
	use_source_name = false;
	output_annotated = false;
	n_totalpaths = 0;
	n_paths = 0;
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
			{"comparedomains",  required_argument,       0, 'C'},
			{"domain",  required_argument, 0, 'd'},
			{"domain2",  required_argument, 0, 'e'},
			{"input",  required_argument, 0, 'i'},
			{"main",  required_argument, 0, 'm'},
			{"output",    required_argument, 0, 'o'},
			{"solver",    required_argument, 0, 's'},
			{"printformula",    no_argument, 0, 'f'},
			{"source-name",    no_argument, 0, 'S'},
			{"annotated",    required_argument, 0, 'a'},
			{"source",    required_argument, 0, 'A'},
			{0, 0, 0, 0}
		};
	/* getopt_long stores the option index here. */
	int option_index = 0;

	 while ((o = getopt_long(argc, argv, "a:ShDi:o:s:cCft:d:e:nNMTm:",long_options,&option_index)) != -1) {
        switch (o) {
        case 'S':
            use_source_name = true;
            break;
        case 'h':
            help = true;
            break;
        case 'D':
            debug = true;
            break;
        case 'c':
            compare = true;
            break;
        case 'C':
            compare_Domain = true;
            break;
        case 't':
            arg = optarg;
			if (setTechnique(arg)) {
				bad_use = true;
			}
            break;
        case 'm':
            arg = optarg;
			if (setMain(arg)) {
				bad_use = true;
			}
            break;
        case 'd':
            arg = optarg;
			if (setApronManager(arg,0)) {
				bad_use = true;
			}
            break;
        case 'e':
            arg = optarg;
			if (setApronManager(arg,1)) {
				bad_use = true;
			}
            break;
        case 'i':
            filename = optarg;
            break;
        case 'n':
			Narrowing[0] = true;
            break;
        case 'N':
			Narrowing[1] = true;
            break;
        case 'T':
			Threshold[0] = true;
            break;
        case 'M':
			Narrowing[0] = true;
			Narrowing[1] = false;
			compare_Narrowing = true;
            break;
        case 'o':
            outputname = optarg;
            break;
        case 'a':
			output_annotated = true;
            annotatedFilename = optarg;
            break;
        case 'A':
            sourceFilename = optarg;
            break;
        case 's':
            arg = optarg;
			if (setSolver(arg)) {
				bad_use = true;
			}
            break;
        case 'f':
            onlyrho = true;
            break;
        case '?':
            std::cout << "Error : Unknown option " << optopt << "\n";
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

