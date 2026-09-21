#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

extern int semantic_error_count;

void semantic_analyze(ASTNode *program);

#endif
