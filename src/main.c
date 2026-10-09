#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "semantic.h"
#include "tac.h"

extern FILE *yyin;
extern int yyparse(void);
extern ASTNode *ast_root;
extern int syntax_error_count;
extern int lexical_error_count;

int main(int argc, char **argv) {
    int show_ast = 0;
    const char *filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ast") == 0) show_ast = 1;
        else filename = argv[i];
    }

    if (!filename) {
        fprintf(stderr, "Usage: %s [--ast] <source-file.ml>\n", argv[0]);
        return 1;
    }

    yyin = fopen(filename, "r");
    if (!yyin) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        return 1;
    }

    printf("==================================================\n");
    printf(" MiniLang Compiler Front End\n");
    printf(" Source: %s\n", filename);
    printf("==================================================\n\n");

    printf("---- Lexical & Syntax Analysis ----\n");
    int parse_result = yyparse();
    fclose(yyin);

    printf("Lexical errors  : %d\n", lexical_error_count);
    printf("Syntax errors   : %d\n", syntax_error_count);

    if (parse_result != 0 || syntax_error_count > 0 || lexical_error_count > 0) {
        printf("\nParsing finished with errors. Skipping semantic analysis.\n");
        return 1;
    }
    printf("Parsing successful.\n\n");

    if (show_ast) {
        printf("---- Abstract Syntax Tree ----\n");
        ast_print(ast_root, 0);
        printf("\n");
    }

    printf("---- Semantic Analysis ----\n");
    semantic_analyze(ast_root);
    printf("Semantic errors : %d\n", semantic_error_count);

    if (semantic_error_count == 0) {
        printf("\nSemantic analysis completed successfully: no errors found.\n");
    } else {
        printf("\nSemantic analysis completed with %d error(s).\n", semantic_error_count);
        ast_free(ast_root);
        return 1;
    }

    if (show_ast) {
        printf("\n---- Type-Annotated AST (after semantic analysis) ----\n");
        ast_print(ast_root, 0);
    }

    printf("\n---- Intermediate Code Generation (Three-Address Code) ----\n");
    tac_generate(ast_root);
    tac_print();

    tac_reset();
    ast_free(ast_root);
    return 0;
}
