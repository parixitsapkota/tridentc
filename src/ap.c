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

static void print_prefix(const char *prefix) {
  if (prefix) {
    fputs(prefix, stdout);
  }
  fputs(FG_BLUE "+-- " RESET, stdout);
}

static char *make_child_prefix(const char *prefix) {
  size_t len = prefix ? strlen(prefix) : 0;
  const char *branch = FG_BLUE "|    " RESET;
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
  case AST_ELSE_IF: fprintf(stdout, "AST_ELSE_IF\n"); break;
  case AST_ELSE: fprintf(stdout, "AST_ELSE\n"); break;
  default: fprintf(stdout, "AST_UNKNOWN\n"); break;
  }

  fprintf(stdout, RESET);
}

static void ap_scope_f(AstPrinter *ap, AstScope *scope, const char *prefix);

static void ap_if_s(AstPrinter *ap, AstNode *curr, const char *prefix) {
  if (!curr) {
    return;
  }

  if (curr->kind == AST_IF) {
    fprintf(stdout, FG_MAGENTA "AST_IF\n" RESET);
  } else if (curr->kind == AST_ELSE_IF) {
    print_prefix(prefix);
    fprintf(stdout, FG_MAGENTA "AST_ELSE_IF\n" RESET);
  } else if (curr->kind == AST_ELSE) {
    print_prefix(prefix);
    fprintf(stdout, FG_MAGENTA "AST_ELSE\n" RESET);
  }

  char *child_prefix = make_child_prefix(prefix);

  if (curr->kind == AST_IF || curr->kind == AST_ELSE_IF) {
    if (child_prefix && curr->if_n) {
      if (curr->if_n->body && curr->if_n->body->kind == AST_SCOPE) {
        ap_scope_f(ap, curr->if_n->body->scope_n, child_prefix);
      }

      free(child_prefix);

      if (curr->if_n->chain) {
        ap_if_s(ap, curr->if_n->chain, prefix);
      }
    } else {
      free(child_prefix);
    }
  } else if (curr->kind == AST_ELSE) {
    if (child_prefix && curr->scope_n) {
      ap_scope_f(ap, curr->scope_n, child_prefix);
    }
    free(child_prefix);
  }
}

static void ap_while_s(AstPrinter *ap, AstNode *curr, const char *prefix) {
  if (!curr) {
    return;
  }

  fprintf(stdout, FG_MAGENTA "AST_WHILE\n" RESET);

  char *child_prefix = make_child_prefix(prefix);

  if (child_prefix && curr->while_n) {
    if (curr->while_n->body && curr->while_n->body->kind == AST_SCOPE) {
      ap_scope_f(ap, curr->while_n->body->scope_n, child_prefix);
    }

    free(child_prefix);
  } else {
    free(child_prefix);
  }
}

static void ap_scope_f(AstPrinter *ap, AstScope *scope, const char *prefix) {
  if (!scope) {
    return;
  }

  AstNode *curr = scope->body;

  while (curr) {
    if (curr->kind == AST_SCOPE) {
      print_prefix(prefix);
      fprintf(stdout, FG_MAGENTA "AST_SCOPE\n" RESET);

      char *child_prefix = make_child_prefix(prefix);
      if (child_prefix) {
        ap_scope_f(ap, curr->scope_n, child_prefix);
        free(child_prefix);
      }

    } else if (curr->kind == AST_IF) {
      print_prefix(prefix);
      ap_if_s(ap, curr, prefix);
    } else if (curr->kind == AST_WHILE) {
      print_prefix(prefix);
      ap_while_s(ap, curr, prefix);
    } else if (curr->kind == AST_LABLE) {
      print_prefix(prefix);
      fprintf(stdout, FG_MAGENTA "AST_LABLE: " FG_GREEN "\"%s\"\n" RESET, curr->lable);
    } else if (curr->kind == AST_GOTO) {
      print_prefix(prefix);
      fprintf(stdout, FG_MAGENTA "AST_GOTO: " FG_GREEN "\"%s\"\n" RESET, curr->lable);
    } else {
      print_prefix(prefix);
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
