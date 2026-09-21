%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylineno;
extern int yylex(void);
extern FILE *yyin;

ASTNode *ast_root = NULL;
int syntax_error_count = 0;

void yyerror(const char *msg);
%}

%locations

%union {
    long        ival;
    double      fval;
    char       *sval;
    DataType    dtype;
    ASTNode    *node;
}

/* keywords */
%token KW_INT KW_FLOAT KW_BOOL KW_VOID
%token KW_IF KW_ELSE KW_WHILE KW_RETURN KW_PRINT KW_TRUE KW_FALSE

/* multi-char operators */
%token EQ NEQ LE GE AND OR

/* literals / identifiers */
%token <sval> ID
%token <ival> INT_LIT
%token <fval> FLOAT_LIT

%type <node> program decl_list decl var_decl_stmt func_decl
%type <node> param_list_opt param_list param block stmt_list stmt
%type <node> assign_stmt if_stmt while_stmt return_stmt print_stmt expr_stmt
%type <node> expr arg_list_opt arg_list
%type <dtype> type

/* precedence, lowest to highest */
%left OR
%left AND
%nonassoc EQ NEQ
%nonassoc '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right '!' UMINUS

%define parse.error verbose

%%

program:
    decl_list { ast_root = $1; }
    ;

decl_list:
      /* empty */          { $$ = ast_make_program(); }
    | decl_list decl        { if ($2) ast_add_child($1, $2); $$ = $1; }
    ;

decl:
      var_decl_stmt
    | func_decl
    | error ';' { yyerrok; $$ = NULL; }
    ;

type:
      KW_INT   { $$ = TYPE_INT; }
    | KW_FLOAT { $$ = TYPE_FLOAT; }
    | KW_BOOL  { $$ = TYPE_BOOL; }
    | KW_VOID  { $$ = TYPE_VOID; }
    ;

var_decl_stmt:
      type ID ';'          { $$ = ast_make_var_decl($1, $2, NULL, @1.first_line); free($2); }
    | type ID '=' expr ';' { $$ = ast_make_var_decl($1, $2, $4, @1.first_line);   free($2); }
    ;

func_decl:
      type ID '(' param_list_opt ')' block
      { $$ = ast_make_func_decl($1, $2, $4, $6, @1.first_line); free($2); }
    ;

param_list_opt:
      /* empty */ { $$ = ast_make_param_list(); }
    | param_list   { $$ = $1; }
    ;

param_list:
      param                  { $$ = ast_make_param_list(); ast_add_child($$, $1); }
    | param_list ',' param   { ast_add_child($1, $3); $$ = $1; }
    ;

param:
    type ID { $$ = ast_make_param($1, $2, @1.first_line); free($2); }
    ;

block:
    '{' stmt_list '}' { $$ = $2; }
    ;

stmt_list:
      /* empty */        { $$ = ast_make_block(); }
    | stmt_list stmt      { if ($2) ast_add_child($1, $2); $$ = $1; }
    ;

stmt:
      var_decl_stmt
    | assign_stmt
    | if_stmt
    | while_stmt
    | return_stmt
    | print_stmt
    | expr_stmt
    | block
    | error ';' { yyerrok; $$ = NULL; }
    ;

assign_stmt:
    ID '=' expr ';' { $$ = ast_make_assign($1, $3, @1.first_line); free($1); }
    ;

if_stmt:
      KW_IF '(' expr ')' block                { $$ = ast_make_if($3, $5, NULL, @1.first_line); }
    | KW_IF '(' expr ')' block KW_ELSE block   { $$ = ast_make_if($3, $5, $7, @1.first_line); }
    ;

while_stmt:
    KW_WHILE '(' expr ')' block { $$ = ast_make_while($3, $5, @1.first_line); }
    ;

return_stmt:
      KW_RETURN ';'      { $$ = ast_make_return(NULL, @1.first_line); }
    | KW_RETURN expr ';' { $$ = ast_make_return($2, @1.first_line); }
    ;

print_stmt:
    KW_PRINT '(' expr ')' ';' { $$ = ast_make_print($3, @1.first_line); }
    ;

expr_stmt:
    expr ';' { $$ = ast_make_expr_stmt($1, @1.first_line); }
    ;

arg_list_opt:
      /* empty */ { $$ = ast_make_param_list(); }
    | arg_list     { $$ = $1; }
    ;

arg_list:
      expr                 { $$ = ast_make_param_list(); ast_add_child($$, $1); }
    | arg_list ',' expr     { ast_add_child($1, $3); $$ = $1; }
    ;

expr:
      expr OR expr    { $$ = ast_make_binop(OR, $1, $3, @1.first_line); }
    | expr AND expr   { $$ = ast_make_binop(AND, $1, $3, @1.first_line); }
    | expr EQ expr    { $$ = ast_make_binop(EQ, $1, $3, @1.first_line); }
    | expr NEQ expr   { $$ = ast_make_binop(NEQ, $1, $3, @1.first_line); }
    | expr '<' expr   { $$ = ast_make_binop('<', $1, $3, @1.first_line); }
    | expr '>' expr   { $$ = ast_make_binop('>', $1, $3, @1.first_line); }
    | expr LE expr    { $$ = ast_make_binop(LE, $1, $3, @1.first_line); }
    | expr GE expr    { $$ = ast_make_binop(GE, $1, $3, @1.first_line); }
    | expr '+' expr   { $$ = ast_make_binop('+', $1, $3, @1.first_line); }
    | expr '-' expr   { $$ = ast_make_binop('-', $1, $3, @1.first_line); }
    | expr '*' expr   { $$ = ast_make_binop('*', $1, $3, @1.first_line); }
    | expr '/' expr   { $$ = ast_make_binop('/', $1, $3, @1.first_line); }
    | expr '%' expr   { $$ = ast_make_binop('%', $1, $3, @1.first_line); }
    | '-' expr %prec UMINUS { $$ = ast_make_unop('-', $2, @1.first_line); }
    | '!' expr              { $$ = ast_make_unop('!', $2, @1.first_line); }
    | '(' expr ')'          { $$ = $2; }
    | ID                    { $$ = ast_make_id($1, @1.first_line); free($1); }
    | ID '(' arg_list_opt ')' { $$ = ast_make_call($1, $3, @1.first_line); free($1); }
    | INT_LIT               { $$ = ast_make_int_lit($1, @1.first_line); }
    | FLOAT_LIT             { $$ = ast_make_float_lit($1, @1.first_line); }
    | KW_TRUE               { $$ = ast_make_bool_lit(1, @1.first_line); }
    | KW_FALSE              { $$ = ast_make_bool_lit(0, @1.first_line); }
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Syntax Error (line %d): %s\n", yylineno, msg);
    syntax_error_count++;
}
