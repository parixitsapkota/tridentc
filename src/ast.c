#include "ast.h"

VarInfo *var_info(Arena *arena, VarKind kind, size_t offset) {
  VarInfo *var = arena_alloc(arena, sizeof(VarInfo));
  *var = (VarInfo){.kind = kind, .offset = offset};
  return var;
}

AstAtom *new_ast_atom(Arena *arena, TokenKind kind, const char *value) {
  AstAtom *atom = arena_alloc(arena, sizeof(AstAtom));
  *atom = (AstAtom){.kind = kind, .value = value};
  return atom;
}

AstUnary *new_ast_unary(Arena *arena, AstNode *node, TokenKind op) {
  AstUnary *unary = arena_alloc(arena, sizeof(AstUnary));
  *unary = (AstUnary){.node = node, .op = op};
  return unary;
}

AstBinary *new_ast_binary(Arena *arena, AstNode *left, TokenKind op, AstNode *right) {
  AstBinary *binary = arena_alloc(arena, sizeof(AstBinary));
  *binary = (AstBinary){.left = left, .op = op, .right = right};
  return binary;
}

AstScope *new_ast_scope(Arena *arena, Hs *symtab, AstScope *parent, AstNode *body) {
  AstScope *scope = arena_alloc(arena, sizeof(AstScope));
  *scope = (AstScope){.symtab = symtab, .parent = parent, .body = body};
  return scope;
}

AstFunction *new_ast_function(Arena *arena, const char *name, Hs *params_tab, size_t params,
                              AstNode *body) {
  AstFunction *function = arena_alloc(arena, sizeof(AstFunction));
  *function =
      (AstFunction){.name = name, .params_tab = params_tab, .params = params, .body = body};
  return function;
}

AstFunctionCall *new_ast_function_call(Arena *arena, const char *name, AstNode *args) {
  AstFunctionCall *function_call = arena_alloc(arena, sizeof(AstFunctionCall));
  *function_call = (AstFunctionCall){.name = name, .args = args};
  return function_call;
}

AstIf *new_ast_conditional(Arena *arena, AstNode *Condition, AstNode *body, AstNode *chain) {
  AstIf *if_n = arena_alloc(arena, sizeof(AstIf));
  *if_n = (AstIf){.Condition = Condition, .body = body, .chain = chain};
  return if_n;
}

AstGlobal *new_ast_global(Arena *arena, const char *name, size_t size) {
  AstGlobal *global_n = arena_alloc(arena, sizeof(AstGlobal));
  *global_n = (AstGlobal){.name = name, .size = size};
  return global_n;
}
