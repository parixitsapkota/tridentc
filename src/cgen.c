#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
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
  return c;
}

void cgen_return_s(Cgen *c) {
  fprintf(c->file, "  mov rdi, %s\t; return code.\n", c->t_node->return_._int_);
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
    case RETURN_NODE: cgen_return_s(c); break;
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