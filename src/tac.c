#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "tac.h"
#include "parser.tab.h"   /* AND, OR, EQ, NEQ, LE, GE token codes */

/* ---- a quad: result = arg1 op arg2  (arg2/op meaning depends on kind, see tac_print) ---- */
typedef struct {
    char *op;      /* operator text, or a pseudo-op: "=", "ifFalse", "goto", "label",
                       "param", "call", "return" */
    char *arg1;
    char *arg2;    /* NULL for unary/copy/label/goto */
    char *result;  /* destination temp/var, or label name for label/goto/ifFalse */
} Quad;

static Quad *quads = NULL;
static int nquads = 0, cap = 0;
static int temp_count = 0;
static int label_count = 0;

static void emit(const char *op, const char *arg1, const char *arg2, const char *result) {
    if (nquads >= cap) {
        cap = cap == 0 ? 64 : cap * 2;
        quads = realloc(quads, sizeof(Quad) * cap);
    }
    quads[nquads].op     = op ? strdup(op) : NULL;
    quads[nquads].arg1   = arg1 ? strdup(arg1) : NULL;
    quads[nquads].arg2   = arg2 ? strdup(arg2) : NULL;
    quads[nquads].result = result ? strdup(result) : NULL;
    nquads++;
}

static char *new_temp(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), "t%d", ++temp_count);
    return strdup(buf);
}

static char *new_label(void) {
    char buf[16];
    snprintf(buf, sizeof(buf), "L%d", ++label_count);
    return strdup(buf);
}

static const char *op_to_str(int op) {
    switch (op) {
        case AND: return "&&";
        case OR:  return "||";
        case EQ:  return "==";
        case NEQ: return "!=";
        case LE:  return "<=";
        case GE:  return ">=";
        default:  { static char c[2]; c[0] = (char)op; c[1] = '\0'; return c; }
    }
}

static char *gen_expr(ASTNode *e);
static void gen_stmt(ASTNode *s);
static void gen_block_stmts(ASTNode *block);

static char *gen_expr(ASTNode *e) {
    if (!e) return strdup("?");
    char buf[64];

    switch (e->kind) {
        case NODE_INT_LIT:
            snprintf(buf, sizeof(buf), "%ld", e->intval);
            return strdup(buf);
        case NODE_FLOAT_LIT:
            snprintf(buf, sizeof(buf), "%g", e->floatval);
            return strdup(buf);
        case NODE_BOOL_LIT:
            return strdup(e->boolval ? "true" : "false");
        case NODE_ID:
            return strdup(e->strval);

        case NODE_BINOP: {
            char *l = gen_expr(e->children[0]);
            char *r = gen_expr(e->children[1]);
            char *t = new_temp();
            emit(op_to_str(e->op), l, r, t);
            free(l); free(r);
            return t;
        }
        case NODE_UNOP: {
            char *v = gen_expr(e->children[0]);
            char *t = new_temp();
            emit(op_to_str(e->op), v, NULL, t);
            free(v);
            return t;
        }
        case NODE_CALL: {
            for (int i = 0; i < e->num_children; i++) {
                char *a = gen_expr(e->children[i]);
                emit("param", a, NULL, NULL);
                free(a);
            }
            char nbuf[16];
            snprintf(nbuf, sizeof(nbuf), "%d", e->num_children);
            char *t = new_temp();
            emit("call", e->strval, nbuf, t);
            return t;
        }
        default:
            return strdup("?");
    }
}

static void gen_stmt(ASTNode *s) {
    if (!s) return;
    switch (s->kind) {
        case NODE_VAR_DECL:
            if (s->num_children > 0) {
                char *v = gen_expr(s->children[0]);
                emit("=", v, NULL, s->strval);
                free(v);
            }
            break;

        case NODE_ASSIGN: {
            char *v = gen_expr(s->children[0]);
            emit("=", v, NULL, s->strval);
            free(v);
            break;
        }

        case NODE_IF: {
            int has_else = s->num_children > 2;
            char *cond = gen_expr(s->children[0]);
            char *Lelse = has_else ? new_label() : NULL;
            char *Lend = new_label();
            emit("ifFalse", cond, NULL, has_else ? Lelse : Lend);
            free(cond);
            gen_block_stmts(s->children[1]);
            if (has_else) {
                emit("goto", NULL, NULL, Lend);
                emit("label", NULL, NULL, Lelse);
                gen_block_stmts(s->children[2]);
                free(Lelse);
            }
            emit("label", NULL, NULL, Lend);
            free(Lend);
            break;
        }

        case NODE_WHILE: {
            char *Lstart = new_label();
            char *Lend = new_label();
            emit("label", NULL, NULL, Lstart);
            char *cond = gen_expr(s->children[0]);
            emit("ifFalse", cond, NULL, Lend);
            free(cond);
            gen_block_stmts(s->children[1]);
            emit("goto", NULL, NULL, Lstart);
            emit("label", NULL, NULL, Lend);
            free(Lstart); free(Lend);
            break;
        }

        case NODE_RETURN: {
            if (s->num_children > 0) {
                char *v = gen_expr(s->children[0]);
                emit("return", v, NULL, NULL);
                free(v);
            } else {
                emit("return", NULL, NULL, NULL);
            }
            break;
        }

        case NODE_PRINT: {
            char *v = gen_expr(s->children[0]);
            emit("print", v, NULL, NULL);
            free(v);
            break;
        }

        case NODE_EXPR_STMT: {
            char *v = gen_expr(s->children[0]);
            free(v); /* result unused as a statement */
            break;
        }

        case NODE_BLOCK:
            gen_block_stmts(s);
            break;

        default:
            break;
    }
}

static void gen_block_stmts(ASTNode *block) {
    for (int i = 0; i < block->num_children; i++) gen_stmt(block->children[i]);
}

static void gen_func_decl(ASTNode *fn) {
    emit("label", NULL, NULL, fn->strval);
    gen_block_stmts(fn->children[1]);
}

void tac_generate(ASTNode *program) {
    for (int i = 0; i < program->num_children; i++) {
        ASTNode *child = program->children[i];
        if (child->kind == NODE_VAR_DECL) gen_stmt(child);
        else if (child->kind == NODE_FUNC_DECL) gen_func_decl(child);
    }
}

void tac_print(void) {
    for (int i = 0; i < nquads; i++) {
        Quad *q = &quads[i];
        if (strcmp(q->op, "label") == 0) {
            printf("%s:\n", q->result);
        } else if (strcmp(q->op, "goto") == 0) {
            printf("    goto %s\n", q->result);
        } else if (strcmp(q->op, "ifFalse") == 0) {
            printf("    ifFalse %s goto %s\n", q->arg1, q->result);
        } else if (strcmp(q->op, "param") == 0) {
            printf("    param %s\n", q->arg1);
        } else if (strcmp(q->op, "call") == 0) {
            printf("    %s = call %s, %s\n", q->result, q->arg1, q->arg2);
        } else if (strcmp(q->op, "return") == 0) {
            if (q->arg1) printf("    return %s\n", q->arg1);
            else printf("    return\n");
        } else if (strcmp(q->op, "print") == 0) {
            printf("    print %s\n", q->arg1);
        } else if (strcmp(q->op, "=") == 0) {
            printf("    %s = %s\n", q->result, q->arg1);
        } else if (q->arg2 == NULL) {
            /* unary op */
            printf("    %s = %s%s\n", q->result, q->op, q->arg1);
        } else {
            printf("    %s = %s %s %s\n", q->result, q->arg1, q->op, q->arg2);
        }
    }
}

void tac_reset(void) {
    for (int i = 0; i < nquads; i++) {
        free(quads[i].op); free(quads[i].arg1); free(quads[i].arg2); free(quads[i].result);
    }
    free(quads);
    quads = NULL;
    nquads = 0; cap = 0; temp_count = 0; label_count = 0;
}
