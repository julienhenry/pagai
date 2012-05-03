#include "Analyzer.h"
#include "SMTlib2driver.h"
#include "SMTlib2parser.hh"

SMTlib2driver::SMTlib2driver() : trace_scanning (false), trace_parsing (false) {
	variables["one"] = 1;
	variables["two"] = 2;
}

SMTlib2driver::~SMTlib2driver() {
}

int SMTlib2driver::parse (FILE* f) {
	file = f;
	scan_begin ();
	yy::SMTlib2parser parser (*this);
	//parser.set_debug_level (trace_parsing);
	*Out << "starting to parse\n";
	int res = parser.parse ();
	*Out << "parse OK\n";
	scan_end ();
	return 1;
}

void SMTlib2driver::error (const yy::location& l, const std::string& m) {
	std::cerr << l << ": " << m << std::endl;
}
     
void SMTlib2driver::error (const std::string& m) {
	std::cerr << m << std::endl;
}
