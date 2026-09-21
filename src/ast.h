#ifndef AST_H
#define AST_H

/* ---- Data types recognized by MiniLang ---- */
typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_VOID
} DataType;

const char *type_name(DataType t);

/* ---- AST node kinds ---- */
typedef enum {
    NODE_PROGRAM,
    NODE_VAR_DECL,      /* decltype, strval=name, children[0]=init expr or NULL */
    NODE_FUNC_DECL,     /* decltype=return type, strval=name,
                           children[0]=NODE_PARAM_LIST, children[1]=NODE_BLOCK */
    NODE_PARAM,         /* decltype=param type, strval=name */
    NODE_PARAM_LIST,    /* children[] = NODE_PARAM */
    NODE_BLOCK,         /* children[] = statements */
    NODE_ASSIGN,        /* strval=target name, children[0]=rhs expr */
    NODE_IF,            /* children[0]=cond, children[1]=then, children[2]=else or NULL */
    NODE_WHILE,         /* children[0]=cond, children[1]=body */
    NODE_RETURN,        /* children[0]=expr or NULL */
    NODE_PRINT,         /* children[0]=expr */
    NODE_EXPR_STMT,     /* children[0]=expr (e.g. bare function call) */
    NODE_BINOP,         /* op=token, children[0]=left, children[1]=right */
    NODE_UNOP,          /* op=token, children[0]=operand */
    NODE_ID,            /* strval=name */
    NODE_INT_LIT,       /* intval */
    NODE_FLOAT_LIT,     /* floatval */
    NODE_BOOL_LIT,      /* boolval */
    NODE_CALL           /* strval=function name, children[]=argument exprs */
} NodeKind;

typedef struct ASTNode {
    NodeKind kind;
    int line;

    /* filled in during semantic analysis for expression nodes */
    DataType dtype;

    /* literal / identifier payloads */
    char   *strval;
    long    intval;
    double  floatval;
    int     boolval;

    /* declaration payload */
    DataType decltype;

    /* operator token (for BINOP/UNOP) */
    int op;

    /* generic children list */
    struct ASTNode **children;
    int num_children;
    int children_cap;
} ASTNode;

ASTNode *ast_new(NodeKind kind, int line);
void ast_add_child(ASTNode *parent, ASTNode *child);

/* convenience constructors */
ASTNode *ast_make_program(void);
ASTNode *ast_make_var_decl(DataType type, char *name, ASTNode *init, int line);
ASTNode *ast_make_func_decl(DataType rettype, char *name, ASTNode *params, ASTNode *body, int line);
ASTNode *ast_make_param(DataType type, char *name, int line);
ASTNode *ast_make_param_list(void);
ASTNode *ast_make_block(void);
ASTNode *ast_make_assign(char *name, ASTNode *rhs, int line);
ASTNode *ast_make_if(ASTNode *cond, ASTNode *thenB, ASTNode *elseB, int line);
ASTNode *ast_make_while(ASTNode *cond, ASTNode *body, int line);
ASTNode *ast_make_return(ASTNode *expr, int line);
ASTNode *ast_make_print(ASTNode *expr, int line);
ASTNode *ast_make_expr_stmt(ASTNode *expr, int line);
ASTNode *ast_make_binop(int op, ASTNode *l, ASTNode *r, int line);
ASTNode *ast_make_unop(int op, ASTNode *operand, int line);
ASTNode *ast_make_id(char *name, int line);
ASTNode *ast_make_int_lit(long v, int line);
ASTNode *ast_make_float_lit(double v, int line);
ASTNode *ast_make_bool_lit(int v, int line);
ASTNode *ast_make_call(char *name, ASTNode *args, int line);

void ast_print(ASTNode *node, int depth);
void ast_free(ASTNode *node);

#endif
