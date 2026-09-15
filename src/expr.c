#include <stdbool.h>

#include "ast.h"
#include "parser.h"
#include "token.h"

bool is_kind_literal(TokenKind kind) {
  switch (kind) {
  case INT:
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
  case ASSIGN: return PREC_ASSIGNMENT;

  case BIT_OR: return PREC_BIT_OR;

  case BIT_AND: return PREC_BIT_AND;

  case LESSER:
  case GREATER:
  case LESSER_EQUAL:
  case GREATER_EQUAL: return PREC_RELATIONAL;

  case EQUAL:
  case NOT_EQUAL: return PREC_EQUALITY;

  case ADD:
  case SUB: return PREC_ADDITIVE;

  case BITSHIFT_L:
  case BITSHIFT_R: return PREC_BITSHIFT;

  case MUL:
  case DEV:
  case MOD: return PREC_MULTIPLICATIVE;

  case COMMA:
  case C_PREN:
  case SEMICOLON: return PREC_NONE;

  default: return PREC_UNKNOWN;
  }
}

AstAtom *parse_atom_f(Parser *p) {
  Token *tok = pconsume(p);
  return new_ast_atom(p->ast, tok->kind, tok->lexeme);
}

AstFunctionCall *parse_function_call_s(Parser *p) {
  const char *name = p->tok->lexeme;
  pconsume(p);

  expect_and_consume(p, O_PREN);
  // TODO: parse_parameters_expr_f
  expect_and_consume(p, C_PREN);

  return new_ast_function_call(p->ast, name);
}

AstNode *parse_left_f(Parser *p) {
  if (curr(p)->kind == O_PREN) {
    expect_and_consume(p, O_PREN);
    AstNode *node = parse_expr_f(p, PREC_NONE);
    expect_and_consume(p, C_PREN);
    return node;
  } else if (curr(p)->kind == IDENTIFIER && curr(p)->next && curr(p)->next->kind == O_PREN) {
    AstNode *left = arena_alloc(p->ast, sizeof(AstNode));
    *left = (AstNode){AST_FUNCTION_CALL, .function_call_n = parse_function_call_s(p)};
    return left;
  } else {
    AstNode *left = arena_alloc(p->ast, sizeof(AstNode));
    *left = (AstNode){AST_ATOM, .atom_n = parse_atom_f(p)};
    return left;
  }
}

AstNode *parse_expr_f(Parser *p, Precedence prec) {
  AstNode *left = parse_left_f(p);

  while (curr(p) != NULL) {
    TokenKind op = curr(p)->kind;
    Precedence op_prec = get_op_prec(op);

    if (op_prec == PREC_UNKNOWN) {
      Token *tok = curr(p);
      fprintf(stderr, "%s:%zu:%zu: Unknown operator `%s`.\n", p->l->file, tok->position.ln,
              tok->position.cn, token_kind_to_str(tok->kind));
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

    *node = (AstNode){AST_BINARY, .binary_n = new_ast_binary(p->ast, left, op, right),
                      .position = position(op_tok->position.ln, op_tok->position.cn)};
    left = node;
  }

  return left;
}
