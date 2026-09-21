#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"
#include "semantic.h"
#include "parser.tab.h"   /* token codes: AND, OR, EQ, NEQ, LE, GE */

int semantic_error_count = 0;
static DataType current_return_type = TYPE_VOID;

static void sem_error(int line, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "Semantic Error (line %d): ", line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    semantic_error_count++;
}

static int is_numeric(DataType t) { return t == TYPE_INT || t == TYPE_FLOAT; }

/* can a value of type src be used where target is expected? (int -> float promotion allowed) */
static int types_assignable(DataType target, DataType src) {
    if (target == src) return 1;
    if (target == TYPE_FLOAT && src == TYPE_INT) return 1;
    return 0;
}

static DataType analyze_expr(ASTNode *e);
static void analyze_stmt(ASTNode *s);
static void analyze_block_stmts(ASTNode *block);
static void analyze_block(ASTNode *block);

static DataType analyze_expr(ASTNode *e) {
    if (!e) return TYPE_UNKNOWN;

    switch (e->kind) {
        case NODE_INT_LIT:
        case NODE_FLOAT_LIT:
        case NODE_BOOL_LIT:
            return e->dtype; /* already set at construction */

        case NODE_ID: {
            Symbol *sym = symtab_lookup(e->strval);
            if (!sym) {
                sem_error(e->line, "undeclared variable '%s'", e->strval);
                e->dtype = TYPE_UNKNOWN;
            } else if (sym->kind == SYM_FUNC) {
                sem_error(e->line, "'%s' is a function, not a variable", e->strval);
                e->dtype = TYPE_UNKNOWN;
            } else {
                e->dtype = sym->type;
            }
            return e->dtype;
        }

        case NODE_CALL: {
            Symbol *sym = symtab_lookup(e->strval);
            /* analyze arguments regardless, to surface errors inside them too */
            DataType argtypes[64];
            int n = e->num_children;
            for (int i = 0; i < n && i < 64; i++) argtypes[i] = analyze_expr(e->children[i]);

            if (!sym) {
                sem_error(e->line, "call to undeclared function '%s'", e->strval);
                e->dtype = TYPE_UNKNOWN;
                return e->dtype;
            }
            if (sym->kind != SYM_FUNC) {
                sem_error(e->line, "'%s' is not a function", e->strval);
                e->dtype = TYPE_UNKNOWN;
                return e->dtype;
            }
            if (n != sym->num_params) {
                sem_error(e->line, "function '%s' expects %d argument(s) but got %d",
                           e->strval, sym->num_params, n);
            } else {
                for (int i = 0; i < n; i++) {
                    if (argtypes[i] == TYPE_UNKNOWN) continue;
                    if (!types_assignable(sym->param_types[i], argtypes[i])) {
                        sem_error(e->line,
                                  "argument %d of call to '%s' has type %s, expected %s",
                                  i + 1, e->strval, type_name(argtypes[i]), type_name(sym->param_types[i]));
                    }
                }
            }
            e->dtype = sym->type;
            return e->dtype;
        }

        case NODE_UNOP: {
            DataType t = analyze_expr(e->children[0]);
            if (e->op == '-') {
                if (t != TYPE_UNKNOWN && !is_numeric(t))
                    sem_error(e->line, "unary '-' requires a numeric operand, got %s", type_name(t));
                e->dtype = (t == TYPE_UNKNOWN) ? TYPE_UNKNOWN : t;
            } else { /* '!' */
                if (t != TYPE_UNKNOWN && t != TYPE_BOOL)
                    sem_error(e->line, "unary '!' requires a bool operand, got %s", type_name(t));
                e->dtype = TYPE_BOOL;
            }
            return e->dtype;
        }

        case NODE_BINOP: {
            DataType lt = analyze_expr(e->children[0]);
            DataType rt = analyze_expr(e->children[1]);
            int op = e->op;

            if (op == AND || op == OR) {
                if (lt != TYPE_UNKNOWN && lt != TYPE_BOOL)
                    sem_error(e->line, "left operand of logical operator must be bool, got %s", type_name(lt));
                if (rt != TYPE_UNKNOWN && rt != TYPE_BOOL)
                    sem_error(e->line, "right operand of logical operator must be bool, got %s", type_name(rt));
                e->dtype = TYPE_BOOL;
            } else if (op == EQ || op == NEQ) {
                if (lt != TYPE_UNKNOWN && rt != TYPE_UNKNOWN) {
                    int ok = (lt == rt) || (is_numeric(lt) && is_numeric(rt));
                    if (!ok)
                        sem_error(e->line, "cannot compare %s with %s", type_name(lt), type_name(rt));
                }
                e->dtype = TYPE_BOOL;
            } else if (op == '<' || op == '>' || op == LE || op == GE) {
                if (lt != TYPE_UNKNOWN && !is_numeric(lt))
                    sem_error(e->line, "relational operator requires numeric operand, got %s", type_name(lt));
                if (rt != TYPE_UNKNOWN && !is_numeric(rt))
                    sem_error(e->line, "relational operator requires numeric operand, got %s", type_name(rt));
                e->dtype = TYPE_BOOL;
            } else { /* + - * / % */
                if (lt != TYPE_UNKNOWN && !is_numeric(lt))
                    sem_error(e->line, "arithmetic operator requires numeric operand, got %s", type_name(lt));
                if (rt != TYPE_UNKNOWN && !is_numeric(rt))
                    sem_error(e->line, "arithmetic operator requires numeric operand, got %s", type_name(rt));
                if (lt == TYPE_UNKNOWN || rt == TYPE_UNKNOWN) e->dtype = TYPE_UNKNOWN;
                else e->dtype = (lt == TYPE_FLOAT || rt == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
            }
            return e->dtype;
        }

        default:
            return TYPE_UNKNOWN;
    }
}

static void analyze_var_decl(ASTNode *n) {
    if (n->decltype == TYPE_VOID) {
        sem_error(n->line, "variable '%s' cannot have type void", n->strval);
    }
    if (symtab_declare_var(n->strval, n->decltype) == -1) {
        sem_error(n->line, "redeclaration of variable '%s' in this scope", n->strval);
    }
    if (n->num_children > 0) {
        DataType itype = analyze_expr(n->children[0]);
        if (itype != TYPE_UNKNOWN && n->decltype != TYPE_VOID && !types_assignable(n->decltype, itype)) {
            sem_error(n->line, "cannot initialize variable '%s' of type %s with value of type %s",
                       n->strval, type_name(n->decltype), type_name(itype));
        }
    }
}

static void analyze_stmt(ASTNode *s) {
    if (!s) return;
    switch (s->kind) {
        case NODE_VAR_DECL:
            analyze_var_decl(s);
            break;

        case NODE_ASSIGN: {
            Symbol *sym = symtab_lookup(s->strval);
            if (!sym) {
                sem_error(s->line, "assignment to undeclared variable '%s'", s->strval);
            } else if (sym->kind == SYM_FUNC) {
                sem_error(s->line, "cannot assign to function '%s'", s->strval);
            } else {
                DataType rtype = analyze_expr(s->children[0]);
                if (rtype != TYPE_UNKNOWN && !types_assignable(sym->type, rtype)) {
                    sem_error(s->line, "cannot assign value of type %s to variable '%s' of type %s",
                               type_name(rtype), s->strval, type_name(sym->type));
                }
            }
            break;
        }

        case NODE_IF: {
            DataType ctype = analyze_expr(s->children[0]);
            if (ctype != TYPE_UNKNOWN && ctype != TYPE_BOOL)
                sem_error(s->line, "if condition must be bool, got %s", type_name(ctype));
            analyze_block(s->children[1]);
            if (s->num_children > 2) analyze_block(s->children[2]);
            break;
        }

        case NODE_WHILE: {
            DataType ctype = analyze_expr(s->children[0]);
            if (ctype != TYPE_UNKNOWN && ctype != TYPE_BOOL)
                sem_error(s->line, "while condition must be bool, got %s", type_name(ctype));
            analyze_block(s->children[1]);
            break;
        }

        case NODE_RETURN: {
            if (s->num_children == 0) {
                if (current_return_type != TYPE_VOID)
                    sem_error(s->line, "function must return a value of type %s", type_name(current_return_type));
            } else {
                DataType rtype = analyze_expr(s->children[0]);
                if (current_return_type == TYPE_VOID) {
                    sem_error(s->line, "void function cannot return a value");
                } else if (rtype != TYPE_UNKNOWN && !types_assignable(current_return_type, rtype)) {
                    sem_error(s->line, "return type mismatch: function returns %s, got %s",
                               type_name(current_return_type), type_name(rtype));
                }
            }
            break;
        }

        case NODE_PRINT:
            analyze_expr(s->children[0]);
            break;

        case NODE_EXPR_STMT:
            analyze_expr(s->children[0]);
            break;

        case NODE_BLOCK:
            analyze_block(s);
            break;

        default:
            break;
    }
}

static void analyze_block_stmts(ASTNode *block) {
    for (int i = 0; i < block->num_children; i++) analyze_stmt(block->children[i]);
}

static void analyze_block(ASTNode *block) {
    symtab_push_scope();
    analyze_block_stmts(block);
    symtab_pop_scope();
}

static void analyze_func_decl(ASTNode *fn) {
    ASTNode *params = fn->children[0];
    ASTNode *body = fn->children[1];

    symtab_push_scope();
    for (int i = 0; i < params->num_children; i++) {
        ASTNode *p = params->children[i];
        if (symtab_declare_var(p->strval, p->decltype) == -1)
            sem_error(p->line, "duplicate parameter name '%s' in function '%s'", p->strval, fn->strval);
    }
    DataType prev = current_return_type;
    current_return_type = fn->decltype;
    analyze_block_stmts(body);
    current_return_type = prev;
    symtab_pop_scope();
}

void semantic_analyze(ASTNode *program) {
    symtab_init();
    current_return_type = TYPE_VOID;

    /* Pass 1: register function signatures first so forward/mutual calls resolve */
    for (int i = 0; i < program->num_children; i++) {
        ASTNode *child = program->children[i];
        if (child->kind == NODE_FUNC_DECL) {
            ASTNode *params = child->children[0];
            int n = params->num_children;
            DataType *ptypes = n > 0 ? malloc(sizeof(DataType) * n) : NULL;
            for (int j = 0; j < n; j++) ptypes[j] = params->children[j]->decltype;
            if (symtab_declare_func(child->strval, child->decltype, ptypes, n) == -1)
                sem_error(child->line, "redeclaration of function '%s'", child->strval);
            free(ptypes);
        }
    }

    /* Pass 2: check global var declarations and function bodies, in source order */
    for (int i = 0; i < program->num_children; i++) {
        ASTNode *child = program->children[i];
        if (child->kind == NODE_VAR_DECL) analyze_var_decl(child);
        else if (child->kind == NODE_FUNC_DECL) analyze_func_decl(child);
    }
}
