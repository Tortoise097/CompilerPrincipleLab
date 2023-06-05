%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>

// Include the Bison generated header file
#include "parser.tab.hpp"

using namespace std;

%}


WhiteSpace      [ \t\n\r]*
LineComment     "//".*
Comment2        "/*"[^"*"]*(\*+[^"*/"][^"*"]*)*\*+"/"

Identifier    [a-zA-Z_][a-zA-Z0-9_]*

Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+



%%

{WhiteSpace}                  { /* ignore */;}
{LineComment}|{Comment2}      { /* ignore */;}

  /*TYPE define*/
"int"           {return INT;}
"const"         {return CONST;}
"void"          {return VOID;}


"return"        {return RETURN;}
"if"            {return IF;}
"else"          {return ELSE;}
"while"         {return WHILE;}
"break"         {return BREAK;}
"continue"      {return CONTINUE;}


"&&" {return AND;}
"||" {return OR;}
"==" {return EQ;}
"!=" {return NE;}
"<=" {return LE;}
">=" {return GE;}


{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { return yytext[0]; }

%%
