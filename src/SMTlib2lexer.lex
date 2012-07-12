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
posinteger	{num}+
posreal		{posinteger}\.{posinteger}
negreal		-{blank}{posreal}
real		{negreal}|{posreal}
neginteger	-{blank}{posinteger}
integer		{neginteger}|{posinteger}
var			{letter}|{num}|_|\.|\%|!
varname		{var}+

%{
# define YY_USER_ACTION  yylloc->columns (yyleng);
%}

%%

%{
  yylloc->step ();
%}
{blank}   yylloc->step ();
[\n]      yylloc->lines (yyleng); yylloc->step ();

"("					return(token::LEFTPAR);			
")"					return(token::RIGHTPAR);		
 /* ")\n"	      yylloc->lines (yyleng); return(token::RIGHTPAR); */		
"/"					return(token::DIVIDE);		
                
"unknown"			return(token::UNKNOWN);			
"unsat"				return(token::UNSAT);			
"sat"				return(token::SAT);
"error"                         return(token::ERROR);
"unsupported"                   return(token::UNSUPPORTED);
"success"                       return(token::SUCCESS);

"true"				return(token::TRUE);			
"false"				return(token::FALSE);			
"model"				return(token::MODEL);			

 /*
"unknown\n"   yylloc->lines (yyleng); return(token::UNKNOWN);			
"unsat\n"     yylloc->lines (yyleng); return(token::UNSAT);			
"sat\n"	      yylloc->lines (yyleng); return(token::SAT);
"model\n"     yylloc->lines (yyleng); return(token::MODEL);
*/			
         
"Int"				return(token::TYPE);			
"Real"				return(token::TYPE);			
"Bool"				return(token::BOOLTYPE);			

"define-fun"		return(token::DEFINEFUN);

{real}				return(token::REALVALUE);

{integer}			return(token::INTVALUE);

{varname}			{
						yylval->sval=new std::string(yytext);
						return(token::VARNAME);
					}

"-"					return(token::MINUS);

.					{
						std::string errormsg("invalid character :");
						errormsg.append(yytext);
						driver.error (*yylloc, errormsg);
					}

<<EOF>>				{yyterminate();}
%%

void SMTlib2driver::scan_begin () {
yy_flex_debug = trace_scanning;
	yyin = file;
}
     
void SMTlib2driver::scan_end () {
	fclose (yyin);
}
