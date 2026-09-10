#ifndef _TRIDENT_AST_H_
#define _TRIDENT_AST_H_

#include <stddef.h>

#include "token.h"
#include "trident.h"

typedef struct AstNode AstNode;

typedef enum {
  UNKNOWN_LIT,
  INT_LIT,
  FLOAT_LIT,
  STRING_LIT,
  CHARACTER_LIT,
  IDENTIFIER_LIT,
} AtomKind;

typedef struct {
  AtomKind kind;
  const char *value;
} AstAtom;

typedef struct {
  AstNode *node;
  TokenKind op;
} AstUnary;

typedef enum {
  OP_NONE,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DEV,
  OP_MOD,
  OP_ASSIGN,
  OP_AND,
  OP_OR,
  OP_XOR,
  OP_EQUAL,
  OP_NOT_EQUAL,
  OP_LESSER,
  OP_GREATER,
  OP_LESSER_EQUAL,
  OP_GREATER_EQUAL,
} OpKind;

typedef struct {
  AstNode *left;
  OpKind op;
  AstNode *right;
} AstBinary;

typedef struct AstScope AstScope;
struct AstScope {
  Hs *symtab;
  AstScope *parent;
  AstNode *body;
};

typedef struct {
  const char *name;
  AstNode *body;
} AstFunction;

typedef struct {
  AstNode *Condition;
  AstNode *body;
} AstConditional;

typedef enum {
  AST_ATOM,
  AST_UNARY,
  AST_BINARY,
  AST_EXPR,
  AST_SCOPE,
  AST_FUNCTION,
  AST_EXTERN,
  AST_AUTO,
  AST_IF,
  AST_ELSE_IF,
  AST_ELSE,
  AST_WHILE,
  AST_RETURN,
} AstKind;

struct AstNode {
  AstKind kind;
  // Token Type_union.
  union {
    AstAtom *atom_n;
    AstUnary *unary_n;
    AstBinary *binary_n;
    AstScope *scope_n;
    AstFunction *function_n;
    AstConditional *conditional_n;
    AstNode *node;
  };
  // Position
  size_t ln;
  size_t cn;
  // for Compound/If statements.
  AstNode *next;
};

typedef struct {
  size_t offset;
} Offset;

AstAtom *new_ast_atom(Arena *arena, AtomKind kind, const char *value);
AstUnary *new_ast_unary(Arena *arena, AstNode *node, TokenKind op);
AstBinary *new_ast_binary(Arena *arena, AstNode *left, OpKind op, AstNode *right);
AstScope *new_ast_scope(Arena *arena, Hs *symtab, AstScope *parent, AstNode *body);
AstFunction *new_ast_function(Arena *arena, const char *name, AstNode *body);
AstConditional *new_ast_if(Arena *arena, AstNode *Condition, AstNode *body);

#endif // _TRIDENT_AST_H_
