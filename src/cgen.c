#include <stdio.h>
#include <stdlib.h>

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

void cgen_expr_f(FILE *file, AstNode *node) {
  if (!node) {
    return;
  }

  if (node->kind == AST_ATOM) {
    switch (node->atom_n.kind) {
    case INT_LIT:
      fprintf(file, "  sub rsp, 4\n");
      fprintf(file, "  mov dword [rsp], %s\n", node->atom_n.value);
      // case IDENTIFIER_LIT:
      //   fprintf(file, "  sub rsp, 4\n");
      //   fprintf(file, "  mov dword [rsp], %d\n", node->atom_n.value);

    default: break;
    }
  } else if (node->kind == AST_BINARY) {
    cgen_expr_f(file, node->binary_n.left);
    cgen_expr_f(file, node->binary_n.right);

    // Left is [rsp+4], Right is [rsp]
    fprintf(file, "  mov eax, [rsp+4]\n");
    fprintf(file, "  mov ebx, [rsp]\n");
    fprintf(file, "  add rsp, 8\n");

    switch (node->binary_n.op) {
    case OP_ADD: fprintf(file, "  add eax, ebx\n"); break;
    case OP_SUB: fprintf(file, "  sub eax, ebx\n"); break;
    case OP_MUL: fprintf(file, "  imul eax, ebx\n"); break;
    case OP_DEV:
      fprintf(file, "  xor edx, edx\n");
      fprintf(file, "  div ebx\n");
      break;
    case OP_MOD:
      fprintf(file, "  xor edx, edx\n");
      fprintf(file, "  div ebx\n");
      fprintf(file, "  mov eax, edx\n");
      break;
    default: break;
    }

    fprintf(file, "  sub rsp, 4\n");
    fprintf(file, "  mov dword [rsp], eax\n");
  }
}

void cgen_return_s(Cgen *c) {
  cgen_expr_f(c->file, c->t_node->node);
  fprintf(c->file, "  mov eax, dword [rsp]\n");
  fprintf(c->file, "  add rsp, 4\n");
}

void cgen_scope_f(Cgen *c, AstNode *first_node) {
  AstNode *c_node = first_node;

  while (c_node != NULL) {
    c->t_node = c_node;
    switch (c_node->kind) {
    case AST_RETURN: cgen_return_s(c); break;
    case AST_EXPR: cgen_expr_f(c->file, c_node->node); break;
    case AST_SCOPE:
      // Handle nested scopes recursively
      cgen_scope_f(c, c_node->node);
      free_hash_set(c_node->symtab);
      break;
    default: break;
    }
    c_node = c_node->next;
  }
}

void cgen_function_s(Cgen *c) {
  fprintf(c->file, "%s:\n", c->t_node->function_n.name);

  AstNode *body_scope = c->t_node->function_n.body;
  if (body_scope && body_scope->kind == AST_SCOPE) {
    cgen_scope_f(c, body_scope->node);
    free_hash_set(body_scope->symtab);
  }

  fprintf(c->file, "  ret\n\n");
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
