%skeleton "lalr1.cc"
%defines "SMTlib2parser.hh"

%define parser_class_name "SMTlib2parser"

%code requires {
     # include <string>
     class SMTlib2driver;
}
%parse-param { SMTlib2driver& driver }
%lex-param   { SMTlib2driver& driver }

%locations
%initial-action
{
  // Initialize the initial location.
  //@$.begin.filename = @$.end.filename = &driver.file;
};

%debug
%error-verbose

%{
# include <cstdlib>
# include <cerrno>
# include <climits>
# include <string>
#include "Analyzer.h"
#include "SMTlib2driver.h"
 
%}

%union
{
  int ival;
  std::string *sval;
};

%token END
%token MODEL
%token DEFINEFUN
%token TRUE FALSE
%token SAT UNSAT UNKNOWN
%token LEFTPAR RIGHTPAR
%token INTVALUE FLOATVALUE
%token TYPE
%token VARNAME

%start Smt

%%

Smt:
	 SAT END					{driver.ans = SAT;}
	| UNSAT END					{driver.ans = UNSAT;}
	| UNKNOWN END				{driver.ans = UNKNOWN;}
	|Model END					{driver.ans = SAT;}
	| END						
	;

Model:
	 LEFTPAR MODEL Model_list RIGHTPAR;

Model_list:
		  DefineFun Model_list
		  | /*empty*/
		  ;

DefineFun:
		 LEFTPAR DEFINEFUN FunName FunArgs FunType FunValue RIGHTPAR;

FunName:
	   VARNAME;

FunArgs:
	   LEFTPAR Argslist RIGHTPAR;

FunValue:
		INTVALUE
		| FLOATVALUE
		| BoolValue

BoolValue:
		 TRUE
		 | FALSE

Argslist:
		/*empty*/
			| VARNAME Argslist;

FunType:
	   TYPE
%%

void yy::SMTlib2parser::error (const yy::SMTlib2parser::location_type& l,
                               const std::string& m) {
       driver.error (l, m);
     }
