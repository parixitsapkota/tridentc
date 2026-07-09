#include <stdbool.h>

#include "ast.h"
#include "parser.h"
#include "token.h"
#include "trident.h"

AtomKind get_atom_kind(TokenKind kind) {
  switch (kind) {
  case INT: return INT_LIT;
  case FLOAT: return FLOAT_LIT;
  case STRING: return STRING_LIT;
  case CHARACTER: return CHARACTER_LIT;

  default: return UNKNOWN_LIT;
  }
}

bool is_kind_literal(TokenKind kind) {
  switch (kind) {
  case INT:
  case FLOAT:
  case STRING:
  case CHARACTER:
  case IDENTIFIER: return true;

  default: return false;
  }
}

bool is_proc_left_Associative(Precedence prec) {
  switch (prec) {
  case PREC_ASSIGNMENT: return false;
  default: return true;
  }
}

Precedence get_op_prec(TokenKind kind) {
  switch (kind) {
  case MUL:
  case DEV:
  case MOD: return PREC_MULTIPLICATIVE;

  case ADD:
  case SUB: return PREC_ADDITIVE;

  case ASSIGN: return PREC_ASSIGNMENT;

  case COMMA: return PREC_COMMA;

  case C_PREN:
  case SEMICOLON: return PREC_NONE;
  default: return PREC_UNKNOWN;
  }
}

OpKind get_op(TokenKind kind) {
  switch (kind) {
  case ADD: return OP_ADD;
  case SUB: return OP_SUB;
  case MUL: return OP_MUL;
  case DEV: return OP_DEV;
  case MOD: return OP_MOD;
  case ASSIGN: return OP_ASSIGN;

  default: return OP_NONE;
  }
}

AstNode *parse_atom_f(Parser *p) {
  Token *tok = pconsume(p);

  AstNode *node = arena_alloc(p->ast, sizeof(AstNode));
  AtomKind literal_kind = get_atom_kind(tok->kind);
  *node = (AstNode){AST_ATOM, .atom_n = (AstAtom){literal_kind, tok->lexeme}, tok->ln, tok->cn};

  return node;
}

AstNode *parse_left_f(Parser *p) {
  if (curr(p)->kind == O_PREN) {
    expect(p, O_PREN);
    AstNode *node = parse_expr_f(p, PREC_NONE);
    expect(p, C_PREN);
    return node;
  } else {
    return parse_atom_f(p);
  }
}

AstNode *parse_expr_f(Parser *p, Precedence prec) {
  AstNode *left = parse_left_f(p);

  while (curr(p) != NULL) {
    TokenKind op = curr(p)->kind;
    Precedence op_prec = get_op_prec(op);

    if (op_prec == PREC_UNKNOWN) {
      Token *tok = curr(p);
      fprintf(stderr, "%s:%zu:%zu: Unknown operator `%d`.\n", p->l->file, tok->ln, tok->cn,
              tok->kind);
    }

    if (op_prec == PREC_NONE || op_prec < prec) {
      break;
    }

    Token *op_tok = pconsume(p);

    AstNode *right;
    if (is_proc_left_Associative(op_prec)) {
      right = parse_expr_f(p, op_prec + 1);
    } else {
      right = parse_expr_f(p, op_prec);
    }

    AstNode *node = arena_alloc(p->ast, sizeof(AstNode));

    *node = (AstNode){AST_BINARY, .binary_n = (AstBinary){left, get_op(op), right}, op_tok->ln,
                      op_tok->cn};
    left = node;
  }

  return left;
}
