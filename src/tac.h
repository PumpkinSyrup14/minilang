#ifndef TAC_H
#define TAC_H

#include "ast.h"

void tac_generate(ASTNode *program);   /* builds the quad list from the AST */
void tac_print(void);                   /* prints generated TAC to stdout */
void tac_reset(void);                   /* frees/reinitializes generator state */

#endif
