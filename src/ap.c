// AST Pretty Printer

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ap.h"
#include "colors.h"

typedef struct {
  Parser *p;
} AstPrinter;

static AstPrinter *init_ap(Parser *p) {
  AstPrinter *ap = malloc(sizeof(*ap));
  if (!ap) {
    return NULL;
  }

  ap->p = p;

  return ap;
}

static void print_prefix(const char *prefix, bool is_last) {
  if (prefix) {
    fputs(prefix, stdout);
  }

  fputs(is_last ? FG_BLUE "`-- " RESET : FG_BLUE "+-- " RESET, stdout);
}

static char *make_child_prefix(const char *prefix, bool is_last) {
  size_t len = prefix ? strlen(prefix) : 0;
  const char *branch = is_last ? "    " : FG_BLUE "|  " RESET;
  size_t branch_len = strlen(branch);

  char *child_prefix = malloc(len + branch_len + 1);
  if (!child_prefix) {
    return NULL;
  }

  if (prefix) {
    memcpy(child_prefix, prefix, len);
  }

  memcpy(child_prefix + len, branch, branch_len);
  child_prefix[len + branch_len] = '\0';

  return child_prefix;
}

static void print_node_name(const AstNode *node) {
  fprintf(stdout, FG_MAGENTA);

  switch (node->kind) {
  case AST_RETURN: fprintf(stdout, "AST_RETURN\n"); break;
  case AST_AUTO: fprintf(stdout, "AST_AUTO\n"); break;
  case AST_EXPR: fprintf(stdout, "AST_EXPR\n"); break;
  case AST_SCOPE: fprintf(stdout, "AST_SCOPE\n"); break;
  case AST_FUNCTION: fprintf(stdout, "AST_FUNCTION\n"); break;
  case AST_IF: fprintf(stdout, "AST_IF\n"); break;
  default: fprintf(stdout, "AST_UNKNOWN\n"); break;
  }

  fprintf(stdout, RESET);
}

static void ap_scope_f(AstPrinter *ap, AstScope *scope, const char *prefix) {
  if (!scope) {
    return;
  }

  AstNode *curr = scope->body;

  while (curr) {
    bool is_last = (curr->next == NULL);

    print_prefix(prefix, is_last);

    if (curr->kind == AST_SCOPE) {
      fprintf(stdout, FG_MAGENTA "AST_SCOPE\n" RESET);

      char *child_prefix = make_child_prefix(prefix, is_last);

      if (child_prefix) {
        ap_scope_f(ap, curr->scope_n, child_prefix);
        free(child_prefix);
      }

    } else if (curr->kind == AST_IF) {
      fprintf(stdout, FG_MAGENTA "AST_IF\n" RESET);

      char *child_prefix = make_child_prefix(prefix, is_last);

      if (child_prefix && curr->conditional_n && curr->conditional_n->body) {
        ap_scope_f(ap, curr->conditional_n->body->scope_n, child_prefix);
      }

      free(child_prefix);

    } else {
      print_node_name(curr);
    }

    curr = curr->next;
  }

  if (scope->symtab) {
    free_hash_set(scope->symtab);
    scope->symtab = NULL;
  }
}

static void ap_function_s(AstPrinter *ap, AstNode *func_node) {
  if (!ap || !func_node || !func_node->function_n) {
    return;
  }

  fprintf(stdout, FG_YELLOW "FUNC " FG_GREEN "\"%s\"\n" RESET, func_node->function_n->name);

  if (!func_node->function_n->body) {
    return;
  }

  ap_scope_f(ap, func_node->function_n->body->scope_n, "");
}

void print_ast(Parser *p) {
  if (!p || !p->ast_head) {
    return;
  }

  AstPrinter *ap = init_ap(p);
  if (!ap) {
    return;
  }

  AstNode *curr = p->ast_head->next;
  while (curr) {
    if (curr->kind == AST_FUNCTION) {
      ap_function_s(ap, curr);
    }

    curr = curr->next;
  }
  fprintf(stdout, RESET);
  free(ap);
}
