#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "cgen.h"

Cgen *init_cgen(Parser *p, const char *file_path) {
  Cgen *c = malloc(sizeof(Cgen));

  if (!c) {
    fprintf(stderr, "FATAL: Failed to allocate Cgen\n");
    exit(EXIT_FAILURE);
  }

  c->p = p;
  c->file_path = file_path;

  c->file = fopen(file_path, "w");
  if (!c->file) {
    fprintf(stderr, "FATAL: Failed to open file: %s\n", file_path);
    free(c);
    exit(EXIT_FAILURE);
  }

  c->if_lable_c = 0;
  c->while_lable_c = 0;

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

    default:
      fprintf(stderr, "DEBUG atom value=%s kind=%d\n", node->atom_n->value, node->atom_n->kind);
      fprintf(stderr, "FATAL: Unhandled atom kind (%d) in cgen_expr_f\n",
              (int)node->atom_n->kind);
      exit(EXIT_FAILURE);
    }

    return;
  }

  if (node->kind == AST_BINARY) {

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
      fprintf(c->file, "  mov dword [rbp - %zu], eax ; var_name : \"%s\"\n", sym->offset * 4,
              var_name);
      return;
    }

    cgen_expr_f(c, node->binary_n->left, scope);
    cgen_expr_f(c, node->binary_n->right, scope);

    fprintf(c->file, "  mov eax, dword [rsp+4]\n");
    fprintf(c->file, "  mov ebx, dword [rsp]\n");
    fprintf(c->file, "  add rsp, 8\n");

    switch (node->binary_n->op) {

    case OP_ADD: fprintf(c->file, "  add eax, ebx\n"); break;
    case OP_SUB: fprintf(c->file, "  sub eax, ebx\n"); break;
    case OP_MUL: fprintf(c->file, "  imul eax, ebx\n"); break;

    case OP_DEV:
      fprintf(c->file, "  cdq\n");
      fprintf(c->file, "  idiv ebx\n");
      break;

    case OP_MOD:
      fprintf(c->file, "  cdq\n");
      fprintf(c->file, "  idiv ebx\n");
      fprintf(c->file, "  mov eax, edx\n");
      break;

    case OP_EQUAL:
      fprintf(c->file, "  cmp eax, ebx\n");
      fprintf(c->file, "  sete al\n");
      fprintf(c->file, "  movzx eax, al\n");
      break;

    case OP_NOT_EQUAL:
      fprintf(c->file, "  cmp eax, ebx\n");
      fprintf(c->file, "  setne al\n");
      fprintf(c->file, "  movzx eax, al\n");
      break;

    case OP_LESSER:
      fprintf(c->file, "  cmp eax, ebx\n");
      fprintf(c->file, "  setl al\n");
      fprintf(c->file, "  movzx eax, al\n");
      break;

    case OP_GREATER:
      fprintf(c->file, "  cmp eax, ebx\n");
      fprintf(c->file, "  setg al\n");
      fprintf(c->file, "  movzx eax, al\n");
      break;

    case OP_LESSER_EQUAL:
      fprintf(c->file, "  cmp eax, ebx\n");
      fprintf(c->file, "  setle al\n");
      fprintf(c->file, "  movzx eax, al\n");
      break;

    case OP_GREATER_EQUAL:
      fprintf(c->file, "  cmp eax, ebx\n");
      fprintf(c->file, "  setge al\n");
      fprintf(c->file, "  movzx eax, al\n");
      break;

    default:
      fprintf(stderr, "FATAL: Unhandled binary op (%d) in cgen_expr_f\n",
              (int)node->binary_n->op);
      exit(EXIT_FAILURE);
    }

    fprintf(c->file, "  sub rsp, 4\n");
    fprintf(c->file, "  mov dword [rsp], eax\n");
    return;
  }

  fprintf(stderr, "FATAL: Unhandled node kind (%d) in cgen_expr_f\n", (int)node->kind);
  exit(EXIT_FAILURE);
}

void cgen_scope_f(Cgen *c, AstScope *scope);

void cgen_auto_s(Cgen *c) {
  fprintf(c->file, "  sub rsp, 4\n");
  return;
}

void cgen_return_s(Cgen *c, AstScope *scope) {
  if (!c->t_node) {
    fprintf(stderr, "FATAL: Invalid return statement\n");
    exit(EXIT_FAILURE);
  }

  if (!c->t_node->node) {
    fprintf(c->file, "  mov rsp, rbp\n");
    fprintf(c->file, "  pop rbp\n");
    fprintf(c->file, "  ret\n");
    return;
  }

  cgen_expr_f(c, c->t_node->node, scope);
  fprintf(c->file, "  mov eax, dword [rsp]\n");
  fprintf(c->file, "  add rsp, 4\n");
  fprintf(c->file, "  mov rsp, rbp\n");
  fprintf(c->file, "  pop rbp\n");
  fprintf(c->file, "  ret\n");
}

static void cgen_if_chain_s(Cgen *c, AstNode *curr, AstScope *scope, size_t end_label) {
  if (!curr) {
    return;
  }

  if (curr->kind == AST_IF || curr->kind == AST_ELSE_IF) {
    size_t next_label = ++(c->if_lable_c);

    cgen_expr_f(c, curr->if_n->Condition, scope);

    fprintf(c->file, "  mov eax, dword [rsp]\n");
    fprintf(c->file, "  add rsp, 4\n");
    fprintf(c->file, "  cmp eax, 0\n");
    fprintf(c->file, "  je .L_if_next_%zu\n", next_label);

    if (curr->if_n->body && curr->if_n->body->kind == AST_SCOPE) {
      cgen_scope_f(c, curr->if_n->body->scope_n);
    }

    fprintf(c->file, "  jmp .L_if_end_%zu\n", end_label);

    fprintf(c->file, ".L_if_next_%zu:\n", next_label);

    if (curr->if_n->chain) {
      cgen_if_chain_s(c, curr->if_n->chain, scope, end_label);
    }
  } else if (curr->kind == AST_ELSE) {
    if (curr->scope_n) {
      cgen_scope_f(c, curr->scope_n);
    }
  }
}

void cgen_if_s(Cgen *c, AstNode *node, AstScope *scope) {
  size_t end_label = ++(c->if_lable_c);

  cgen_if_chain_s(c, node, scope, end_label);

  fprintf(c->file, ".L_if_end_%zu:\n", end_label);
}

void cgen_while_s(Cgen *c, AstNode *node, AstScope *scope) {
  size_t label_id = ++(c->while_lable_c);

  fprintf(c->file, ".L_while_condition_%zu:\n", label_id);

  cgen_expr_f(c, node->while_n->Condition, scope);

  fprintf(c->file, "  mov eax, dword [rsp]\n");
  fprintf(c->file, "  add rsp, 4\n");
  fprintf(c->file, "  cmp eax, 0\n");
  fprintf(c->file, "  je .L_while_end_%zu\n", label_id);

  if (node->while_n->body && node->while_n->body->kind == AST_SCOPE) {
    cgen_scope_f(c, node->while_n->body->scope_n);
  }

  fprintf(c->file, "  jmp .L_while_condition_%zu\n", label_id);
  fprintf(c->file, ".L_while_end_%zu:\n", label_id);
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

    case AST_AUTO: cgen_auto_s(c); break;

    case AST_EXPR:
      cgen_expr_f(c, curr->node, scope);
      fprintf(c->file, "  add rsp, 4\n");
      break;

    case AST_SCOPE: cgen_scope_f(c, curr->scope_n); break;

    case AST_IF: cgen_if_s(c, curr, curr->if_n->body->scope_n->parent); break;

    case AST_WHILE: cgen_while_s(c, curr, curr->while_n->body->scope_n->parent); break;

    case AST_LABLE: fprintf(c->file, ".L_%s:\n", curr->lable); break;

    case AST_GOTO: fprintf(c->file, "  jmp .L_%s\n", curr->lable); break;

    default: break;
    }
    curr = curr->next;
  }

  if (scope->symtab) {
    free_hash_set(scope->symtab);
  }
}

void cgen_function_s(Cgen *c) {
  c->if_lable_c = 0;

  fprintf(c->file, "%s:\n", c->t_node->function_n->name);
  fprintf(c->file, "  push rbp\n");
  fprintf(c->file, "  mov rbp, rsp\n");

  AstNode *save_func = c->t_node;
  AstScope *body_scope = save_func->function_n->body->scope_n;

  cgen_scope_f(c, body_scope);
  fprintf(c->file, "\n");

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
  fprintf(c->file, "global mainCRTStartup\n"
  fprintf(c->file, "extern ExitProcess\n");

#endif

  c->t_node = c->p->ast_head->next;
  while (c->t_node != NULL) {
    if (c->t_node->kind == AST_FUNCTION) {
      cgen_function_s(c);
    } else {
      fprintf(stderr, "FAITAL : Invalid statement!\n");
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
  if (!c) {
    return;
  }
  if (c->file) {
    fclose(c->file);
  }
  free(c);
}
