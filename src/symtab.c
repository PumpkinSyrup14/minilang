#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

static Scope *current = NULL;

void symtab_init(void) {
    current = NULL;
    symtab_push_scope(); /* global scope */
}

void symtab_push_scope(void) {
    Scope *s = calloc(1, sizeof(Scope));
    s->parent = current;
    s->symbols = NULL;
    current = s;
}

void symtab_pop_scope(void) {
    if (!current) return;
    Scope *old = current;
    current = old->parent;
    Symbol *sym = old->symbols;
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym->param_types);
        free(sym);
        sym = next;
    }
    free(old);
}

Symbol *symtab_lookup_current(const char *name) {
    if (!current) return NULL;
    for (Symbol *s = current->symbols; s; s = s->next)
        if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

Symbol *symtab_lookup(const char *name) {
    for (Scope *sc = current; sc; sc = sc->parent)
        for (Symbol *s = sc->symbols; s; s = s->next)
            if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

int symtab_declare_var(const char *name, DataType type) {
    if (symtab_lookup_current(name)) return -1;
    Symbol *s = calloc(1, sizeof(Symbol));
    s->name = strdup(name);
    s->kind = SYM_VAR;
    s->type = type;
    s->next = current->symbols;
    current->symbols = s;
    return 0;
}

int symtab_declare_func(const char *name, DataType rettype, DataType *param_types, int num_params) {
    if (symtab_lookup_current(name)) return -1;
    Symbol *s = calloc(1, sizeof(Symbol));
    s->name = strdup(name);
    s->kind = SYM_FUNC;
    s->type = rettype;
    s->num_params = num_params;
    if (num_params > 0) {
        s->param_types = malloc(sizeof(DataType) * num_params);
        memcpy(s->param_types, param_types, sizeof(DataType) * num_params);
    }
    s->next = current->symbols;
    current->symbols = s;
    return 0;
}
