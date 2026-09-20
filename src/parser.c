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

AstNode *parse_scope_f(Parser *p, AstScope *parent, size_t parent_stack_offset, Hs *symtab);

Parser *init_parser(Lexer *l) {
  Parser *p = malloc(sizeof(Parser));
  p->i = 0;
  p->l = l;
  p->ast = init_arena(sizeof(AstNode) * TOKENS_STORE);
  p->var_info = init_arena(sizeof(VarInfo) * 1024);
  p->global_table = init_hash_set(20);
  p->functions_table = init_hash_set(24);
  p->global_scope_n = new_ast_scope(p->ast, p->global_table, NULL, NULL);
  return p;
}

AstNode *parse_auto_s(Parser *p, Hs *symtab, size_t *stack_offset) {
  expect_and_consume(p, AUTO);
  size_t var_size = 0;

  do {
    VarInfo *info = var_info(p->var_info, AUTO_VAR, *stack_offset);
    put_to_hash_set(symtab, p->tok->lexeme, info);
    ++*stack_offset;
    pconsume(p);
    ++var_size;
    if (is_kind(p, COMMA)) {
      pconsume(p);
    }
  } while (!is_kind(p, SEMICOLON));
  expect_and_consume(p, SEMICOLON);

  AstNode *auto_n = arena_alloc(p->ast, sizeof(AstNode));
  *auto_n = (AstNode){.kind = AST_AUTO, .auto_var_size = var_size};
  return auto_n;
}

AstNode *parse_if_s(Parser *p, AstKind statenemt_kind, AstScope *parent,
                    size_t parent_stack_offset) {
  expect_and_consume(p, IF);

  expect_and_consume(p, O_PREN);
  AstNode *condition = parse_expr_f(p, PREC_NONE);
  expect_and_consume(p, C_PREN);

  Hs *if_symtab = init_hash_set(16);
  AstNode *body = parse_scope_f(p, parent, parent_stack_offset, if_symtab);

  // parse chain
  AstNode *chain = NULL;
  if (is_kind(p, ELSE)) {
    pconsume(p);
    if (is_kind(p, IF)) {
      chain = parse_if_s(p, AST_ELSE_IF, parent, parent_stack_offset);
    } else {
      Hs *else_symtab = init_hash_set(16);
      AstNode *else_body = parse_scope_f(p, parent, parent_stack_offset, else_symtab);

      chain = arena_alloc(p->ast, sizeof(AstNode));
      *chain = (AstNode){AST_ELSE, .scope_n = else_body->scope_n};
    }
  }

  AstNode *if_wraper = arena_alloc(p->ast, sizeof(AstNode));

  AstIf *if_n = arena_alloc(p->ast, sizeof(AstIf));
  *if_n = (AstIf){.Condition = condition, .body = body, .chain = chain};

  *if_wraper = (AstNode){statenemt_kind, .if_n = if_n};
  return if_wraper;
}

AstNode *parse_while_s(Parser *p, AstScope *parent, size_t parent_stack_offset) {
  Hs *symtab = init_hash_set(16);
  expect_and_consume(p, WHILE);

  expect_and_consume(p, O_PREN);
  AstNode *condition = parse_expr_f(p, PREC_NONE);
  expect_and_consume(p, C_PREN);

  AstNode *body = parse_scope_f(p, parent, parent_stack_offset, symtab);

  AstNode *while_wraper = arena_alloc(p->ast, sizeof(AstNode));

  AstWhile *while_n = arena_alloc(p->ast, sizeof(AstWhile));
  *while_n = (AstWhile){.Condition = condition, .body = body};

  *while_wraper = (AstNode){AST_WHILE, .while_n = while_n};
  return while_wraper;
}

AstNode *parse_goto_s(Parser *p) {
  expect_and_consume(p, GOTO);

  const char *jump_lable_name = p->tok->lexeme;
  pconsume(p);

  AstNode *goto_n = arena_alloc(p->ast, sizeof(AstNode));
  *goto_n = (AstNode){AST_GOTO, .lable = jump_lable_name};
  expect_and_consume(p, SEMICOLON);
  return goto_n;
}

AstNode *parse_return_s(Parser *p) {
  expect_and_consume(p, RETURN);

  AstNode *expr_n = parse_expr_f(p, PREC_NONE);

  AstNode *return_n = arena_alloc(p->ast, sizeof(AstNode));
  *return_n = (AstNode){AST_RETURN, .node = expr_n};
  expect_and_consume(p, SEMICOLON);
  return return_n;
}

void parse_global_s(Parser *p, const char *name) {
  size_t size = 1;

  goto first;

comma:

  name = p->tok->lexeme;
  pconsume(p);

first: {
  VarInfo *info = var_info(p->var_info, GLOBAL_VAR, 0);
  put_to_hash_set(p->global_table, name, info);
}

  if (is_kind(p, O_BRACKET)) {
    pconsume(p);
    size = atoi(p->tok->lexeme);
    expect_and_consume(p, C_BRACKET);
  }

  AstNode *global_n = arena_alloc(p->ast, sizeof(AstNode));
  *global_n = (AstNode){AST_GLOBAL, .global_n = new_ast_global(p->ast, name, size)};
  add_node(&p->t_node, global_n);

  if (is_kind(p, COMMA)) {
    pconsume(p);
    goto comma;
  }

  expect_and_consume(p, SEMICOLON);
}

AstNode *parse_expr_s(Parser *p) {
  if (is_kind(p, COMMA)) {
    pconsume(p);
    return NULL;
  }
  AstNode *p_expr_n = parse_expr_f(p, PREC_NONE);

  AstNode *expr_n = arena_alloc(p->ast, sizeof(AstNode));
  *expr_n = (AstNode){AST_EXPR, .node = p_expr_n};
  expect_and_consume(p, SEMICOLON);
  return expr_n;
}

AstNode *parse_lable_s(Parser *p) {
  const char *lable_name = p->tok->lexeme;
  pconsume(p);
  pconsume(p);
  AstNode *lable_n = arena_alloc(p->ast, sizeof(AstNode));
  *lable_n = (AstNode){AST_LABLE, .lable = lable_name};
  return lable_n;
}

AstNode *parse_statements_f(Parser *p, AstScope *parent, Hs *symtab, size_t *stack_offset) {
  switch (p->tok->kind) {
  case RETURN: return parse_return_s(p); break;

  case AUTO: return parse_auto_s(p, symtab, stack_offset); break;

  case O_BRACE: {
    Hs *symtab = init_hash_set(16);
    return parse_scope_f(p, parent, *stack_offset, symtab);
  }

  case IF: return parse_if_s(p, AST_IF, parent, *stack_offset);

  case WHILE: return parse_while_s(p, parent, *stack_offset);

  case GOTO: return parse_goto_s(p); break;

  case IDENTIFIER:
    if (ppeak(p) && ppeak(p)->kind == COLON) {
      return parse_lable_s(p);
      break;
    }

  default: break;
  }
  return parse_expr_s(p);
}

AstNode *parse_scope_f(Parser *p, AstScope *parent, size_t parent_stack_offset, Hs *symtab) {
  AstScope *scope_n = arena_alloc(p->ast, sizeof(AstScope));
  size_t stack_offset = parent_stack_offset;
  AstNode *body_head = arena_alloc(p->ast, sizeof(AstNode));
  AstNode *body_tail = body_head;

  expect_and_consume(p, O_BRACE);
  while (p->tok != NULL) {
    if (p->tok->kind == C_BRACE) {
      break;
    }
    AstNode *statement = parse_statements_f(p, scope_n, symtab, &stack_offset);
    if (statement) {
      add_node(&body_tail, statement);
    }
  }
  expect_and_consume(p, C_BRACE);

  *scope_n = (AstScope){.symtab = symtab, .parent = parent, .body = body_head->next};
  AstNode *body_n = arena_alloc(p->ast, sizeof(AstNode));
  *body_n = (AstNode){AST_SCOPE, .scope_n = scope_n};
  return body_n;
}

AstNode *parse_function_s(Parser *p, const char *name) {
  size_t stack_offset = 1, params = 0;
  AstNode *body_head = arena_alloc(p->ast, sizeof(AstNode));
  AstNode *body_tail = body_head;
  Hs *symtable = init_hash_set(24);
  AstScope *params_tab_scope = new_ast_scope(p->ast, symtable, p->global_scope_n, NULL);

  expect_and_consume(p, O_PREN);

  while (p->tok->kind != C_PREN) {
    const char *parm_name = p->tok->lexeme;
    pconsume(p);

    VarInfo *info = var_info(p->var_info, PARAM_VAR, stack_offset);
    put_to_hash_set(symtable, parm_name, info);
    ++stack_offset;
    ++params;

    if (is_kind(p, COMMA)) {
      pconsume(p);
    }
  }

  expect_and_consume(p, C_PREN);
  add_node(&body_tail, parse_statements_f(p, params_tab_scope, symtable, &stack_offset));

  AstNode *function_n = arena_alloc(p->ast, sizeof(AstNode));
  *function_n = (AstNode){AST_FUNCTION, .function_n = new_ast_function(p->ast, name, symtable,
                                                                       params, body_head->next)};

  return function_n;
}

void parser(Parser *p) {
  p->ast_head = arena_alloc(p->ast, sizeof(AstNode));
  p->t_node = p->ast_head;

  p->tok = p->l->tok_head->next;
  while (p->tok != NULL) {
    if (p->tok->kind == IDENTIFIER) {
      const char *name = p->tok->lexeme;
      pconsume(p);
      if (p->tok->kind == O_PREN) {
        add_node(&p->t_node, parse_function_s(p, name));
      } else {
        parse_global_s(p, name);
      }
    } else {
      fprintf(stderr, "%s:%zu:%zu: Unexpected token `%s`.\n", p->l->file, p->tok->position.ln,
              p->tok->position.cn, token_kind_to_str(p->tok->kind));
      pconsume(p);
    }
  }
}

void free_parser(Parser *p) {
  free_arena(p->ast);
  free_arena(p->var_info);
  free_hash_set(p->global_table);
  free_hash_set(p->functions_table);
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
  if (p->tok && p->tok->kind == kind) {
    return 1;
  }
  return 0;
}
