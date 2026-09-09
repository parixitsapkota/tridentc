#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"
#include "token.h"
#include "trident.h"

// Parser helper funcions
Token *ppeak(const Parser *p);
Token *pconsume(Parser *p);
void expect_and_consume(Parser *p, TokenKind kind);
void add_node(AstNode **t_node, AstNode *node);
Token *curr(const Parser *p);

Parser *init_parser(Lexer *l) {
  Parser *p = malloc(sizeof(Parser));
  p->i = 0;
  p->l = l;
  p->ast = init_arena(sizeof(AstNode) * TOKENS_STORE);
  p->offsets = init_arena(sizeof(Offset) * 1024);
  return p;
}

void parse_auto_s(Parser *p, Hs *symtab, size_t *stack_offset, AstNode **body_tail) {
  expect_and_consume(p, AUTO);

auto_alloc: {
  Offset *offset = arena_alloc(p->offsets, sizeof(Offset));
  *offset = (Offset){*stack_offset};
  put_to_hash_set(symtab, p->tok->lexeme, offset);
  ++*stack_offset;

  AstNode *auto_n = arena_alloc(p->ast, sizeof(AstNode));
  *auto_n = (AstNode){.kind = AST_AUTO, .node = NULL};
  add_node(body_tail, auto_n);

  pconsume(p);
}
  if (is_kind(p, COMMA)) {
    pconsume(p);
    goto auto_alloc;
  }

  expect_and_consume(p, SEMICOLON);
}

AstNode *parse_return_s(Parser *p) {
  expect_and_consume(p, RETURN);

  AstNode *expr_n = parse_expr_f(p, PREC_NONE);

  AstNode *return_n = arena_alloc(p->ast, sizeof(AstNode));
  *return_n = (AstNode){AST_RETURN, .node = expr_n};
  expect_and_consume(p, SEMICOLON);
  return return_n;
}

AstNode *parse_scope_f(Parser *p, AstScope *parent, size_t parent_stack_offset) {
  AstScope *scope_n = arena_alloc(p->ast, sizeof(AstScope));
  AstNode *body_head = arena_alloc(p->ast, sizeof(AstNode));
  AstNode *body_tail = body_head;
  size_t stack_offset = parent_stack_offset;
  Hs *symtab = init_hash_set(16);

  expect_and_consume(p, O_BRACE);
  while (p->tok != NULL) {
    if (p->tok->kind == C_BRACE) {
      break;
    }
    switch (p->tok->kind) {
    case RETURN: add_node(&body_tail, parse_return_s(p)); break;
    case AUTO: parse_auto_s(p, symtab, &stack_offset, &body_tail); break;
    case O_BRACE: add_node(&body_tail, parse_scope_f(p, scope_n, stack_offset)); break;
    default: {
      AstNode *p_expr_n = parse_expr_f(p, PREC_NONE);
      AstNode *expr_n = arena_alloc(p->ast, sizeof(AstNode));
      *expr_n = (AstNode){AST_EXPR, .node = p_expr_n};
      expect_and_consume(p, SEMICOLON);
      add_node(&body_tail, expr_n);
    }
    }
  }
  expect_and_consume(p, C_BRACE);

  *scope_n = (AstScope){.symtab = symtab, .parent = parent, .body = body_head->next};
  AstNode *body_n = arena_alloc(p->ast, sizeof(AstNode));
  *body_n = (AstNode){AST_SCOPE, .scope_n = scope_n};
  return body_n;
}

AstNode *parse_function_s(Parser *p) {
  const char *function_name = p->tok->lexeme;
  pconsume(p);

  expect_and_consume(p, O_PREN);
  // TODO: parse_parameters_f
  expect_and_consume(p, C_PREN);

  AstNode *body_n = parse_scope_f(p, NULL, 1);

  AstNode *function_n = arena_alloc(p->ast, sizeof(AstNode));
  *function_n =
      (AstNode){AST_FUNCTION, .function_n = new_ast_function(p->ast, function_name, body_n)};

  return function_n;
}

void parser(Parser *p) {
  p->ast_head = arena_alloc(p->ast, sizeof(AstNode));
  p->t_node = p->ast_head;

  p->tok = p->l->tok_head->next;
  while (p->tok != NULL) {
    if (p->tok->kind == IDENTIFIER) {
      add_node(&p->t_node, parse_function_s(p));
    } else {
      fprintf(stderr, "%s:%zu:%zu: Unexpected token `%s`.\n", p->l->file, p->tok->ln, p->tok->cn,
              token_kind_to_str(p->tok->kind));
      pconsume(p);
    }
  }
}

void free_parser(Parser *p) {
  free_arena(p->ast);
  free_arena(p->offsets);
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

void expect_and_consume(Parser *p, TokenKind kind) {
  const Token *tok = pconsume(p);
  if (tok == NULL) {
    fprintf(stderr, "%s:%zu:%zu: Expected `%s` but got end of input\n", p->l->file, p->l->ln,
            p->l->cn, token_kind_to_str(kind));
    return;
  }
  const TokenKind got = tok->kind;
  if (kind != got) {
    fprintf(stderr, "%s:%zu:%zu: Expected `%s` but got `%s`\n", p->l->file, p->l->ln, p->l->cn,
            token_kind_to_str(kind), token_kind_to_str(got));
  }
}

bool is_kind(Parser *p, TokenKind kind) {
  if (p->tok->kind == kind) {
    return 1;
  }
  return 0;
}
