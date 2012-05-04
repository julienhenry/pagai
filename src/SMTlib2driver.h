#ifndef SMTLIB2DRIVER_H
# define SMTLIB2DRIVER_H
# include <string>
# include <map>
# include "SMTlib2parser.hh"


// Tell Flex the lexer's prototype ...
# define YY_DECL                                        \
  yy::SMTlib2parser::token_type                         \
  yylex (yy::SMTlib2parser::semantic_type* yylval,      \
         yy::SMTlib2parser::location_type* yylloc,      \
         SMTlib2driver& driver)
// ... and declare it for the parser's sake.
YY_DECL;

typedef enum {
	SAT,
	UNSAT,
	UNKNOWN
} SMTans;

// Conducting the whole scanning and parsing of Calc++.
class SMTlib2driver
{
	public:
		SMTlib2driver ();
		virtual ~SMTlib2driver ();

		std::set<std::string> model;

		SMTans ans;

		// Handling the scanner.
		void scan_begin ();
		void scan_end ();
		bool trace_scanning;

		// Run the parser.  Return 0 on success.
		int parse (FILE* f);
		FILE* file;
		bool trace_parsing;

		// Error handling.
       void error (const yy::location& l, const std::string& m);
       void error (const std::string& m);

};
#endif
