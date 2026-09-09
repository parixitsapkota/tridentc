#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "cgen.h"

Cgen *init_cgen(Parser *p, const char *file_path) {
  Cgen *c = malloc(sizeof(Cgen));
  c->p = p;
  c->file_path = file_path;
  c->file = fopen(file_path, "w");
  if (!c->file) {
    fprintf(stderr, "FATAL : Failed to open file: %s\n", file_path);
    exit(EXIT_FAILURE);
  }
  c->alloc_c = 0;
  return c;
}

static Offset *lookup_symbol(AstScope *scope, const char *name) {
  AstScope *curr_scope = scope;
  while (curr_scope != NULL) {
    if (curr_scope->symtab && has_in_hash_set(curr_scope->symtab, name)) {
      return (Offset *)get_from_hash_set(curr_scope->symtab, name);
    }
    curr_scope = curr_scope->parent;
  }
  return NULL;
}

void cgen_expr_f(Cgen *c, AstNode *node, AstScope *scope) {
  if (!node) {
    return;
  }

  if (node->kind == AST_ATOM) {
    switch (node->atom_n->kind) {
    case INT_LIT:
      fprintf(c->file, "  sub rsp, 4\n");
      fprintf(c->file, "  mov dword [rsp], %s\n", node->atom_n->value);
      break;

    case IDENTIFIER_LIT: {
      Offset *sym = lookup_symbol(scope, node->atom_n->value);
      if (!sym) {
        fprintf(stderr, "FATAL: Undefined variable '%s'\n", node->atom_n->value);
        exit(EXIT_FAILURE);
      }

      fprintf(c->file, "  mov eax, dword [rbp - %zu]\n", sym->offset * 4);
      fprintf(c->file, "  sub rsp, 4\n");
      fprintf(c->file, "  mov dword [rsp], eax\n");
      break;
    }

    default: break;
    }
  } else if (node->kind == AST_BINARY) {
    if (node->binary_n->op == OP_ASSIGN) {
      cgen_expr_f(c, node->binary_n->right, scope);

      AstNode *left_node = node->binary_n->left;

      const char *var_name = left_node->atom_n->value;

      Offset *sym = lookup_symbol(scope, var_name);
      if (!sym) {
        fprintf(stderr, "FATAL: Undefined variable '%s'\n", var_name);
        exit(EXIT_FAILURE);
      }

      fprintf(c->file, "  mov eax, dword [rsp]\n");
      fprintf(c->file, "  add rsp, 4\n");
      fprintf(c->file, "  mov dword [rbp - %zu], eax\n", sym->offset * 4);
      return;
    }

    cgen_expr_f(c, node->binary_n->left, scope);
    cgen_expr_f(c, node->binary_n->right, scope);

    // Left is [rsp+4], Right is [rsp]
    fprintf(c->file, "  mov eax, [rsp+4]\n");
    fprintf(c->file, "  mov ebx, [rsp]\n");
    fprintf(c->file, "  add rsp, 8\n");

    switch (node->binary_n->op) {
    case OP_ADD: fprintf(c->file, "  add eax, ebx\n"); break;
    case OP_SUB: fprintf(c->file, "  sub eax, ebx\n"); break;
    case OP_MUL: fprintf(c->file, "  imul eax, ebx\n"); break;
    case OP_DEV:
      fprintf(c->file, "  xor edx, edx\n");
      fprintf(c->file, "  div ebx\n");
      break;
    case OP_MOD:
      fprintf(c->file, "  xor edx, edx\n");
      fprintf(c->file, "  cdq\n");
      fprintf(c->file, "  idiv ebx\n");
      fprintf(c->file, "  mov eax, edx\n");
      break;
    default: break;
    }

    fprintf(c->file, "  sub rsp, 4\n");
    fprintf(c->file, "  mov dword [rsp], eax\n");
  }
}

void cgen_auto_s(Cgen *c, AstNode *node, AstScope *scope) {
  if (!node->node) {
    return;
  }

  if (node->node->kind == AST_BINARY && node->node->binary_n->op == OP_ASSIGN) {
    cgen_expr_f(c, node->node, scope);
  } else if (node->node->kind == AST_EXPR) {
    cgen_expr_f(c, node->node->node, scope);
  }
}

void cgen_return_s(Cgen *c, AstScope *scope) {
  cgen_expr_f(c, c->t_node, scope);
  fprintf(c->file, "  mov eax, dword [rsp]\n");
  fprintf(c->file, "  add rsp, 4\n\n");
}

void cgen_scope_f(Cgen *c, AstScope *scope) {
  if (!scope) {
    return;
  }

  AstNode *curr = scope->body;

  while (curr != NULL) {
    c->t_node = curr;

    switch (curr->kind) {
    case AST_RETURN: cgen_return_s(c, scope); break;
    case AST_AUTO: cgen_auto_s(c, curr, scope); break;
    case AST_EXPR: cgen_expr_f(c, curr->node, scope); break;
    case AST_SCOPE: cgen_scope_f(c, curr->scope_n); break;
    default: break;
    }

    curr = curr->next;
  }

  free_hash_set(scope->symtab);
}

void cgen_function_s(Cgen *c) {
  fprintf(c->file, "%s:\n", c->t_node->function_n->name);

  // Stack Frame Prologue
  fprintf(c->file, "  push rbp\n");
  fprintf(c->file, "  mov rbp, rsp\n\n");

  AstNode *save_func = c->t_node;
  AstScope *body_scope = save_func->function_n->body->scope_n;

  cgen_scope_f(c, body_scope);

  // Stack Frame Epilogue
  fprintf(c->file, "  mov rsp, rbp\n");
  fprintf(c->file, "  pop rbp\n");
  fprintf(c->file, "  ret\n\n");

  c->t_node = save_func->next;
}

void cgen(Cgen *c) {
  fprintf(c->file, "; MODULE : %s\n", c->p->l->file);

#if defined(__linux__) || defined(_TUX)
  fprintf(c->file, "global _start\n");

#elif defined(__MacOS__) || defined(_XOS)
  fprintf(c->file, "global _main\n");
  fprintf(c->file, "extern _exit\n");

#elif defined(__FreeBSD__) || defined(_BSD)
  fprintf(c->file, "global _start\n");

#elif defined(__Windows__) || defined(_WIN32)
  fprintf(c->file, "global mainCRTStartup\n");
  fprintf(c->file, "extern ExitProcess\n");
#endif

  c->t_node = c->p->ast_head->next;
  while (c->t_node != NULL) {
    if (c->t_node->kind == AST_FUNCTION) {
      cgen_function_s(c);
    } else {
      c->t_node = c->t_node->next;
    }
  }

#if defined(__linux__) || defined(_TUX)
  fprintf(c->file, "_start:\n");
  fprintf(c->file, "  call main\n");
  fprintf(c->file, "  mov rdi, rax\n");
  fprintf(c->file, "  mov rax, 0x3C\n");
  fprintf(c->file, "  syscall\n");

#elif defined(__MacOS__) || defined(_XOS)
  fprintf(c->file, "_main:\n");
  fprintf(c->file, "  call _main_impl\n");
  fprintf(c->file, "  mov rdi, rax\n");
  fprintf(c->file, "  call _exit\n");

#elif defined(__FreeBSD__) || defined(_BSD)
  fprintf(c->file, "_start:\n");
  fprintf(c->file, "  call main\n");
  fprintf(c->file, "  mov rdi, rax\n");
  fprintf(c->file, "  mov rax, 1\n");
  fprintf(c->file, "  sysenter\n");

#elif defined(__Windows__) || defined(_WIN32)
  fprintf(c->file, "mainCRTStartup:\n");
  fprintf(c->file, "  sub rsp, 40\n");
  fprintf(c->file, "  call main\n");
  fprintf(c->file, "  mov rcx, rax\n");
  fprintf(c->file, "  call ExitProcess\n");
#endif
}

void free_cgen(Cgen *c) {
  fclose(c->file);
  free(c);
}
