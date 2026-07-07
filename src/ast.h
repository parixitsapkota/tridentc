#ifndef _TRIDENT_AST_H_
#define _TRIDENT_AST_H_

#include "token.h"
#include <stddef.h>

typedef struct AstNode AstNode;

typedef enum {
  UNKNOWN_LITERAL,
  INT_LITERAL,
  FLOAT_LITERAL,
  STRING_LITERAL,
  CHARACTER_LITERAL,
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
} OpKind;

typedef struct {
  AstNode *left;
  OpKind op;
  AstNode *right;
} AstBinary;

typedef enum {
  AST_ATOM,
  AST_UNARY,
  AST_BINARY,
  AST_EXPR,
  AST_RETURN,
} AstKind;

struct AstNode {
  AstKind kind;
  // Token Type_union.
  union {
    AstAtom atom_n;
    AstBinary binary_n;
    AstUnary unary_n;
    AstNode *node;
  };
  // Position
  size_t ln;
  size_t cn;
  // for compound statements.
  AstNode *next;
};

typedef enum {
  PREC_UNKNOWN = -1,
  PREC_NONE = 0,
  PREC_COMMA,
  PREC_ASSIGNMENT,
  PREC_RANGE,
  PREC_OR,
  PREC_XOR,
  PREC_AND,
  PREC_BIT_OR,
  PREC_BIT_XOR,
  PREC_BIT_AND,
  PREC_RELATIVE,
  PREC_COMPARITIVE,
  PREC_BITSHIFT,
  PREC_ADDITIVE,
  PREC_MULTIPLICATIVE,
} Precedence;

#endif // _TRIDENT_AST_H_
