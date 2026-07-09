#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "trident.h"

// Parser helper funcions
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

AstNode *parse_let_s(Parser *p) {
  expect(p, LET);

  AstNode *expr_n = parse_expr_f(p, PREC_NONE);

  AstNode *let_n = arena_alloc(p->ast, sizeof(AstNode));
  *let_n = (AstNode){AST_LET, .node = expr_n};
  expect(p, SEMICOLON);
  return let_n;
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
    switch (p->tok->kind) {
    case RETURN: add_node(&p->t_node, parse_return_s(p)); break;
    case LET: add_node(&p->t_node, parse_let_s(p)); break;
    default:
      fprintf(stderr, "%s:%zu:%zu: Unexpected token `%d`.\n", p->l->file, p->tok->ln, p->tok->cn,
              p->tok->kind);
      pconsume(p);
    }
  }
}

void free_parser(Parser *p) {
  free_arena(p->ast);
  free(p);
}

void add_node(AstNode **t_node, AstNode *node) {
  node->next = NULL;
  (*t_node)->next = node;
  *t_node = node;
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
