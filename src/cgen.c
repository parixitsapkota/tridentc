#include <stdio.h>
#include <stdlib.h>

#include "trident.h"

Cgen *init_cgen(Parser *p, const char *file_path) {
  Cgen *c = malloc(sizeof(Cgen));
  c->p = p;
  c->file_path = file_path;
  c->file = fopen(file_path, "wa");
  if (!c->file) {
    fprintf(stderr, "FATAL : Failed to open file: %s\n", file_path);
    exit(EXIT_FAILURE);
  }
  c->alloc_c = 0;
  return c;
}

void cgen_expr_f(FILE *file, AstNode *node) {
  if (node->kind == AST_ATOM) {
    fprintf(file, "  sub rsp, 4 ; allocate stack.\n");
    fprintf(file, "  mov dword [rsp], %s ; load value.\n", node->atom_n.value);
  } else if (node->kind == AST_BINARY) {
    cgen_expr_f(file, node->binary_n.left);
    cgen_expr_f(file, node->binary_n.right);

    fprintf(file, "  mov eax, [rsp+4] ; Left value.\n");
    fprintf(file, "  mov ebx, [rsp] ; Right value.\n");
    fprintf(file, "  add rsp, 8 ; Clean up last two values.\n");

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

    fprintf(file, "  sub rsp, 4 ; allocate stack.\n");
    fprintf(file, "  mov [rsp], eax ; load result back onto stack.\n");
  }
}

void cgen_return_s(Cgen *c) {
  cgen_expr_f(c->file, c->t_node->node);
  fprintf(c->file, "  mov rdi, [rsp]\t; return value.\n");
  fprintf(c->file, "  add rsp, 4 ; deallocate result.\n");
  c->t_node = c->t_node->next;
}

void cgen(Cgen *c) {
  // Generate header
  fprintf(c->file, "; MODEULE : %s\n", c->p->l->file);
  fprintf(c->file, "global _start\n");
  fprintf(c->file, "section .text\n\n");

  fprintf(c->file, "_start:\n");
  c->t_node = c->p->ast_head->next;
  while (c->t_node != NULL) {
    switch (c->t_node->kind) {
    case AST_RETURN: cgen_return_s(c); break;
    default: c->t_node = c->t_node->next;
    }
  }
  fprintf(c->file, "  mov rax, 0x3C\t; exit syscall.\n");
  fprintf(c->file, "  syscall\t; syscall intrupt.\n");
}

void free_cgen(Cgen *c) {
  fclose(c->file);
  free(c);
}