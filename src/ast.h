#ifndef _TRIDENT_AST_H_
#define _TRIDENT_AST_H_

typedef struct AstNode AstNode;

typedef struct {
  const char *_int_;
} AstReturn;

typedef enum {
  RETURN_NODE,
} AstKind;

struct AstNode {
  AstKind kind;
  union {
    AstReturn return_;
  };
  AstNode *next;
};

#endif // _TRIDENT_AST_H_
