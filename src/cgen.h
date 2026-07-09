#ifndef _TRIDENT_CGEN_H_
#define _TRIDENT_CGEN_H_

#include "ast.h"
#include "parser.h"
#include "token.h"

// Codegen Structure
typedef struct {
  Parser *p;
  // file store
  const char *file_path;
  FILE *file;
  // Allocation counter
  size_t alloc_c; // Expr stack allocation count;
  // Helper/Temp vars
  AstNode *t_node;
} Cgen;

/// Returns a Cgen context based on given Parser context.
Cgen *init_cgen(Parser *p, const char *file_path);
/// generates asm based on the given context and mutates the cgen state accordingly.
void cgen(Cgen *c);
/// Frees the allocated memory in the cgen context.
void free_cgen(Cgen *c);

#endif // _TRIDENT_CGEN_H_
