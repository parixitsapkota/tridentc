#include "ast.h"

AstAtom *new_ast_atom(Arena *arena, AtomKind kind, const char *value) {
  AstAtom *atom = arena_alloc(arena, sizeof(AstAtom));
  *atom = (AstAtom){.kind = kind, .value = value};
  return atom;
}

AstUnary *new_ast_unary(Arena *arena, AstNode *node, TokenKind op) {
  AstUnary *unary = arena_alloc(arena, sizeof(AstUnary));
  *unary = (AstUnary){.node = node, .op = op};
  return unary;
}

AstBinary *new_ast_binary(Arena *arena, AstNode *left, OpKind op, AstNode *right) {
  AstBinary *binary = arena_alloc(arena, sizeof(AstBinary));
  *binary = (AstBinary){.left = left, .op = op, .right = right};
  return binary;
}

AstScope *new_ast_scope(Arena *arena, Hs *symtab, AstNode *parent, AstNode *body) {
  AstScope *scope = arena_alloc(arena, sizeof(AstScope));
  *scope = (AstScope){.symtab = symtab, .parent = parent, .body = body};
  return scope;
}

AstFunction *new_ast_function(Arena *arena, const char *name, AstNode *body) {
  AstFunction *function = arena_alloc(arena, sizeof(AstFunction));
  *function = (AstFunction){.name = name, .body = body};
  return function;
}
