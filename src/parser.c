#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "trident.h"

// Lexer helper funcions
Token *ppeak(const Parser *p);
Token *pconsume(Parser *p);
void expect(Parser *p, TokenKind kind);
void add_node(AstNode **t_node, AstNode *node);
Token *curr(const Parser *p);

Parser *init_parser(Lexer *l) {
  Parser *p = malloc(sizeof(Parser));
  p->i = 0;
  p->l = l;
  p->ast = init_arena(sizeof(AstNode) * TOKENS_STORE);
  return p;
}

AtomKind get_atom_kind(TokenKind kind) {
  switch (kind) {
  case INT: return INT_LITERAL;
  case FLOAT: return FLOAT_LITERAL;
  case STRING: return STRING_LITERAL;
  case CHARACTER: return CHARACTER_LITERAL;

  default: return UNKNOWN_LITERAL;
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

  case SEMICOLON: return PREC_NONE;

  default: return PREC_UNKNOWN;
  }
}

OpKind get_op(TokenKind kind) {
  switch (kind) {
  case MUL: return OP_MUL;
  case DEV: return OP_DEV;
  case MOD: return OP_MOD;
  case ADD: return OP_ADD;
  case SUB: return OP_SUB;
  default: return OP_NONE;
  }
}

AstNode *parse_atom_f(Parser *p) {
  Token *tok = pconsume(p);
  if (is_kind_literal(tok->kind)) {
    AstNode *node = arena_alloc(p->ast, sizeof(AstNode));
    AtomKind literal_kind = get_atom_kind(tok->kind);
    *node = (AstNode){AST_ATOM, .atom_n = (AstAtom){literal_kind, tok->lexeme}, tok->ln, tok->cn};
    return node;
  }

  return NULL;
}

AstNode *parse_expr_f(Parser *p, Precedence prec) {
  AstNode *left = parse_atom_f(p);

  while (ppeak(p) != NULL) {
    TokenKind op = curr(p)->kind;
    Precedence op_prec = get_op_prec(op);

    if (op_prec == PREC_UNKNOWN) {
      Token *tok = ppeak(p);
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

AstNode *parse_return_s(Parser *p) {
  expect(p, RETURN);

  AstNode *expr_n = parse_expr_f(p, PREC_NONE);

  AstNode *return_n = arena_alloc(p->ast, sizeof(AstNode));
  *return_n = (AstNode){AST_RETURN, .node = expr_n};
  expect(p, SEMICOLON);
  return return_n;
}

void parser(Parser *p) {
  p->ast_head = arena_alloc(p->ast, sizeof(AstNode));
  p->t_node = p->ast_head;

  p->tok = p->l->tok_head->next;
  while (p->tok != NULL) {
    if (p->tok->lexeme) {
      printf(BOLD FG_BLUE "%s:%zu:%zu:" RESET "\t%-3d : %s\n", p->l->file, p->tok->ln, p->tok->cn,
             p->tok->kind, p->tok->lexeme);
    }

    switch (p->tok->kind) {
    case RETURN: add_node(&p->t_node, parse_return_s(p)); break;
    default: pconsume(p);
    }
  }
}

void add_node(AstNode **t_node, AstNode *node) {
  node->next = NULL;
  (*t_node)->next = node;
  *t_node = node;
}

void free_parser(Parser *p) {
  free_arena(p->ast);
  free(p);
}

Token *ppeak(const Parser *p) {
  if (p->tok != NULL && p->tok->next != NULL) {
    return p->tok->next;
  }
  return NULL;
}

Token *curr(const Parser *p) {
  if (p->tok != NULL) {
    return p->tok;
  }
  return NULL;
}

Token *pconsume(Parser *p) {
  Token *current = p->tok;
  p->tok = ppeak(p);
  return current;
}

void expect(Parser *p, TokenKind kind) {
  const Token *tok = pconsume(p);
  if (tok == NULL) {
    fprintf(stderr, "%s:%zu:%zu: Expected `%d` but got end of input\n", p->l->file, p->l->ln,
            p->l->cn, kind);
    return;
  }
  const TokenKind got = tok->kind;
  if (kind != got) {
    fprintf(stderr, "%s:%zu:%zu: Expected `%d` but got `%d`\n", p->l->file, p->l->ln, p->l->cn,
            kind, got);
  }
}
