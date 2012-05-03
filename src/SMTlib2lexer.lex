%{
# include <cstdlib>
# include <cerrno>
# include <climits>
# include <string>

#include "Analyzer.h"
#include "SMTlib2driver.h"
#include "SMTlib2parser.hh"

typedef yy::SMTlib2parser::token token;

/* Work around an incompatibility in flex (at least versions
        2.5.31 through 2.5.33): it generates code that does
        not conform to C89.  See Debian bug 333231
        <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.  */
# undef yywrap
# define yywrap() 1

#define yyterminate() return token::END

%}
%option noyywrap nounput batch debug

blank       [\t ]
letter      [A-Za-z]
num			[0-9]
integer		{num}+
var			{letter}|{num}|_|\.|\%
varname		{var}+

%{
# define YY_USER_ACTION  yylloc->columns (yyleng);
%}

%%

%{
  yylloc->step ();
%}
{blank}+   yylloc->step ();
[\n]+      yylloc->lines (yyleng); yylloc->step ();

"("					return(token::LEFTPAR);			{*Out << "LEFTPAR\n";}
")"					return(token::RIGHTPAR);		{*Out << "RIGHTPAR\n";}

"unknown"			return(token::UNKNOWN);				{*Out << "UNKNOWN\n";}
"unsat"				return(token::UNSAT);				{*Out << "UNSAT\n";}
"sat"				return(token::SAT);					{*Out << "SAT\n";}
"true"				return(token::TRUE);				{*Out << "TRUE\n";}
"false"				return(token::FALSE);				{*Out << "FALSE\n";}
"model"				return(token::MODEL);				{*Out << "MODEL\n";}

"Int"				return(token::TYPE);				{*Out << "TYPE INT\n";}
"Bool"				return(token::TYPE);				{*Out << "TYPE BOOL\n";}

"define-fun"		return(token::DEFINEFUN);

{integer}			{
						*Out << "integer\n";
						long n = strtol (yytext, NULL, 10);
						if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE))
							driver.error (*yylloc, "integer is out of range");
						yylval->ival = n;
						return(token::INTVALUE);
					}

{varname}			{
						*Out << "varname\n";
						yylval->sval=new std::string(yytext);
						return(token::VARNAME);
					}


%%

void SMTlib2driver::scan_begin () {
yy_flex_debug = trace_scanning;
	yyin = file;
}
     
void SMTlib2driver::scan_end () {
	fclose (yyin);
}
