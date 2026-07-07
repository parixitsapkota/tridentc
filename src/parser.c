#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "trident.h"

// Lexer helper funcions
Token *ppeak(const Parser *p);
Token *pconsume(Parser *p);
void expect(Parser *p, TokenKind kind);
void add_node(AstNode *t_node, AstNode *node);

Parser *init_parser(Lexer *l) {
  Parser *p = malloc(sizeof(Parser));
  p->i = 0;
  p->l = l;
  p->ast = init_arena(sizeof(AstNode) * TOKENS_STORE);
  return p;
}

AstNode *parse_return_s(Parser *p) {
  expect(p, RETURN);
  AstNode *return_ = arena_alloc(p->ast, sizeof(AstNode));
  Token *val = pconsume(p);
  const char *_int_ = val != NULL ? val->lexeme : NULL;
  *return_ = (AstNode){RETURN_NODE, .return_ = (AstReturn){_int_}};
  expect(p, SEMICOLON);
  return return_;
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
    case RETURN: add_node(p->t_node, parse_return_s(p)); break;
    default: pconsume(p);
    }
  }
}

void add_node(AstNode *t_node, AstNode *node) {
  node->next = NULL;
  t_node->next = node;
  t_node = node;
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
