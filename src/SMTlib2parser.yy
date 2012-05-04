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
#include <sstream>
#include "Analyzer.h"
#include "SMTlib2driver.h"
 
%}

%union
{
  int ival;
  bool bval;
  double fval;
  std::string *sval;
};

%token END 0
%token MODEL
%token DEFINEFUN
%token<bval> TRUE FALSE
%token SAT UNSAT UNKNOWN
%token LEFTPAR RIGHTPAR
%token<bval> INTVALUE 
%token<bval> FLOATVALUE
%token<bval> TYPE
%token BOOLTYPE
%token<sval> VARNAME

%start Smt

%type<sval> FunName
%type<bval> FunValue
%type<bval> FunType
%type<bval> BoolValue

%%

Smt:
	 SAT 					{driver.ans = SAT;}
	|UNSAT					{driver.ans = UNSAT;}
	|UNKNOWN 				{driver.ans = UNKNOWN;}
	|Model 					{driver.ans = SAT;}
	;

Model:
	 LEFTPAR MODEL Model_list RIGHTPAR;

Model_list:
		  DefineFun Model_list
		  | /*empty*/
		  ;

DefineFun:
		 LEFTPAR DEFINEFUN FunName FunArgs FunType FunValue RIGHTPAR
							{
								if ($5 && $6) {
									driver.model.insert(*$3);
									}
							}
		 ;

FunName:
	   VARNAME				{$$ = $1;}
	   ;

FunArgs:
	   LEFTPAR Argslist RIGHTPAR;

FunValue:
		INTVALUE						{$$ = false;}
		| FLOATVALUE					{$$ = false;}
		| BoolValue						{$$ = $1;}
		| LEFTPAR FunValue RIGHTPAR		{$$ = $2;}
		;

BoolValue:
		 TRUE							{$$ = true;}
		 | FALSE						{$$ = false;}
		 ;

Argslist:
		/*empty*/
			| VARNAME Argslist;

FunType:
	   BOOLTYPE							{$$ = true;}
	   | TYPE							{$$ = false;}
	   ;
%%

void yy::SMTlib2parser::error (const yy::SMTlib2parser::location_type& l,
                               const std::string& m) {
       driver.error (l, m);
     }
