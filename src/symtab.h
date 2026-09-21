#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"

typedef enum { SYM_VAR, SYM_FUNC } SymKind;

typedef struct Symbol {
    char *name;
    SymKind kind;
    DataType type;        /* var type, or function return type */

    /* function-only fields */
    DataType *param_types;
    int num_params;

    struct Symbol *next;   /* linked list within a scope */
} Symbol;

typedef struct Scope {
    Symbol *symbols;
    struct Scope *parent;
} Scope;

void symtab_init(void);
void symtab_push_scope(void);
void symtab_pop_scope(void);

/* returns 0 on success, -1 if already declared in the CURRENT scope */
int symtab_declare_var(const char *name, DataType type);
int symtab_declare_func(const char *name, DataType rettype, DataType *param_types, int num_params);

/* searches current scope outward; NULL if not found */
Symbol *symtab_lookup(const char *name);

/* searches only the innermost/current scope (used for duplicate checks) */
Symbol *symtab_lookup_current(const char *name);

#endif
