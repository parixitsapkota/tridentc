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
} OpKind;

typedef struct {
  AstNode *left;
  OpKind op;
  AstNode *right;
} AstBinary;

typedef struct {
  const char *name;
  AstNode *body;
} AstFunction;

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
    AstAtom atom_n;
    AstUnary unary_n;
    AstBinary binary_n;
    AstFunction function_n;
    AstNode *node;
  };
  // Position
  size_t ln;
  size_t cn;
  // for compound statements.
  AstNode *next;
  // Variable stack position
  size_t stack_offset;
};

typedef struct {
  size_t offset;
} Offset;

#endif // _TRIDENT_AST_H_
