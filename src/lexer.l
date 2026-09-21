%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int lexical_error_count = 0;

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.last_line  = yylineno;
%}

%option noyywrap yylineno

DIGIT   [0-9]
ID_HEAD [a-zA-Z_]
ID_TAIL [a-zA-Z0-9_]

%%

"//"[^\n]*              { /* line comment: skip */ }
[ \t\r]+                { /* whitespace: skip */ }
\n                       { /* newline: yylineno auto-incremented (yylineno option) */ }

"int"    { return KW_INT; }
"float"  { return KW_FLOAT; }
"bool"   { return KW_BOOL; }
"void"   { return KW_VOID; }
"if"     { return KW_IF; }
"else"   { return KW_ELSE; }
"while"  { return KW_WHILE; }
"return" { return KW_RETURN; }
"print"  { return KW_PRINT; }
"true"   { return KW_TRUE; }
"false"  { return KW_FALSE; }

"=="  { return EQ; }
"!="  { return NEQ; }
"<="  { return LE; }
">="  { return GE; }
"&&"  { return AND; }
"||"  { return OR; }

[-+*/%=<>!(){};,]  { return yytext[0]; }

{ID_HEAD}{ID_TAIL}*  { yylval.sval = strdup(yytext); return ID; }

{DIGIT}+"."{DIGIT}+"."{DIGIT}+  {
    fprintf(stderr, "Lexical Error (line %d): malformed number literal '%s'\n", yylineno, yytext);
    lexical_error_count++;
}

{DIGIT}+"."{DIGIT}+ { yylval.fval = atof(yytext); return FLOAT_LIT; }
{DIGIT}+             { yylval.ival = atol(yytext); return INT_LIT; }

.  {
    fprintf(stderr, "Lexical Error (line %d): unexpected character '%s'\n", yylineno, yytext);
    lexical_error_count++;
}

%%
