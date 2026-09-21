#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

const char *type_name(DataType t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

ASTNode *ast_new(NodeKind kind, int line) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = kind;
    n->line = line;
    n->dtype = TYPE_UNKNOWN;
    n->children = NULL;
    n->num_children = 0;
    n->children_cap = 0;
    return n;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->num_children >= parent->children_cap) {
        parent->children_cap = parent->children_cap == 0 ? 4 : parent->children_cap * 2;
        parent->children = realloc(parent->children, sizeof(ASTNode *) * parent->children_cap);
    }
    parent->children[parent->num_children++] = child;
}

ASTNode *ast_make_program(void) {
    return ast_new(NODE_PROGRAM, 0);
}

ASTNode *ast_make_var_decl(DataType type, char *name, ASTNode *init, int line) {
    ASTNode *n = ast_new(NODE_VAR_DECL, line);
    n->decltype = type;
    n->strval = strdup(name);
    if (init) ast_add_child(n, init);
    return n;
}

ASTNode *ast_make_func_decl(DataType rettype, char *name, ASTNode *params, ASTNode *body, int line) {
    ASTNode *n = ast_new(NODE_FUNC_DECL, line);
    n->decltype = rettype;
    n->strval = strdup(name);
    ast_add_child(n, params);
    ast_add_child(n, body);
    return n;
}

ASTNode *ast_make_param(DataType type, char *name, int line) {
    ASTNode *n = ast_new(NODE_PARAM, line);
    n->decltype = type;
    n->strval = strdup(name);
    return n;
}

ASTNode *ast_make_param_list(void) {
    return ast_new(NODE_PARAM_LIST, 0);
}

ASTNode *ast_make_block(void) {
    return ast_new(NODE_BLOCK, 0);
}

ASTNode *ast_make_assign(char *name, ASTNode *rhs, int line) {
    ASTNode *n = ast_new(NODE_ASSIGN, line);
    n->strval = strdup(name);
    ast_add_child(n, rhs);
    return n;
}

ASTNode *ast_make_if(ASTNode *cond, ASTNode *thenB, ASTNode *elseB, int line) {
    ASTNode *n = ast_new(NODE_IF, line);
    ast_add_child(n, cond);
    ast_add_child(n, thenB);
    if (elseB) ast_add_child(n, elseB);
    return n;
}

ASTNode *ast_make_while(ASTNode *cond, ASTNode *body, int line) {
    ASTNode *n = ast_new(NODE_WHILE, line);
    ast_add_child(n, cond);
    ast_add_child(n, body);
    return n;
}

ASTNode *ast_make_return(ASTNode *expr, int line) {
    ASTNode *n = ast_new(NODE_RETURN, line);
    if (expr) ast_add_child(n, expr);
    return n;
}

ASTNode *ast_make_print(ASTNode *expr, int line) {
    ASTNode *n = ast_new(NODE_PRINT, line);
    ast_add_child(n, expr);
    return n;
}

ASTNode *ast_make_expr_stmt(ASTNode *expr, int line) {
    ASTNode *n = ast_new(NODE_EXPR_STMT, line);
    ast_add_child(n, expr);
    return n;
}

ASTNode *ast_make_binop(int op, ASTNode *l, ASTNode *r, int line) {
    ASTNode *n = ast_new(NODE_BINOP, line);
    n->op = op;
    ast_add_child(n, l);
    ast_add_child(n, r);
    return n;
}

ASTNode *ast_make_unop(int op, ASTNode *operand, int line) {
    ASTNode *n = ast_new(NODE_UNOP, line);
    n->op = op;
    ast_add_child(n, operand);
    return n;
}

ASTNode *ast_make_id(char *name, int line) {
    ASTNode *n = ast_new(NODE_ID, line);
    n->strval = strdup(name);
    return n;
}

ASTNode *ast_make_int_lit(long v, int line) {
    ASTNode *n = ast_new(NODE_INT_LIT, line);
    n->intval = v;
    n->dtype = TYPE_INT;
    return n;
}

ASTNode *ast_make_float_lit(double v, int line) {
    ASTNode *n = ast_new(NODE_FLOAT_LIT, line);
    n->floatval = v;
    n->dtype = TYPE_FLOAT;
    return n;
}

ASTNode *ast_make_bool_lit(int v, int line) {
    ASTNode *n = ast_new(NODE_BOOL_LIT, line);
    n->boolval = v;
    n->dtype = TYPE_BOOL;
    return n;
}

ASTNode *ast_make_call(char *name, ASTNode *args, int line) {
    ASTNode *n = ast_new(NODE_CALL, line);
    n->strval = strdup(name);
    if (args) {
        for (int i = 0; i < args->num_children; i++)
            ast_add_child(n, args->children[i]);
        free(args->children);
        free(args);
    }
    return n;
}

static const char *kind_name(NodeKind k) {
    switch (k) {
        case NODE_PROGRAM: return "Program";
        case NODE_VAR_DECL: return "VarDecl";
        case NODE_FUNC_DECL: return "FuncDecl";
        case NODE_PARAM: return "Param";
        case NODE_PARAM_LIST: return "ParamList";
        case NODE_BLOCK: return "Block";
        case NODE_ASSIGN: return "Assign";
        case NODE_IF: return "If";
        case NODE_WHILE: return "While";
        case NODE_RETURN: return "Return";
        case NODE_PRINT: return "Print";
        case NODE_EXPR_STMT: return "ExprStmt";
        case NODE_BINOP: return "BinOp";
        case NODE_UNOP: return "UnOp";
        case NODE_ID: return "Id";
        case NODE_INT_LIT: return "IntLit";
        case NODE_FLOAT_LIT: return "FloatLit";
        case NODE_BOOL_LIT: return "BoolLit";
        case NODE_CALL: return "Call";
    }
    return "?";
}

void ast_print(ASTNode *node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s", kind_name(node->kind));

    switch (node->kind) {
        case NODE_VAR_DECL:
            printf(" (%s %s)", type_name(node->decltype), node->strval);
            break;
        case NODE_FUNC_DECL:
            printf(" (%s %s)", type_name(node->decltype), node->strval);
            break;
        case NODE_PARAM:
            printf(" (%s %s)", type_name(node->decltype), node->strval);
            break;
        case NODE_ASSIGN:
        case NODE_ID:
        case NODE_CALL:
            printf(" (%s)", node->strval);
            break;
        case NODE_INT_LIT:
            printf(" (%ld)", node->intval);
            break;
        case NODE_FLOAT_LIT:
            printf(" (%g)", node->floatval);
            break;
        case NODE_BOOL_LIT:
            printf(" (%s)", node->boolval ? "true" : "false");
            break;
        case NODE_BINOP:
        case NODE_UNOP:
            printf(" (op=%d)", node->op);
            break;
        default:
            break;
    }
    if (node->dtype != TYPE_UNKNOWN) printf("  : %s", type_name(node->dtype));
    printf("\n");

    for (int i = 0; i < node->num_children; i++) {
        ast_print(node->children[i], depth + 1);
    }
}

void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) ast_free(node->children[i]);
    free(node->children);
    free(node->strval);
    free(node);
}
