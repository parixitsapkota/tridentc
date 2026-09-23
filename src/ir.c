#include "ir.h"
#include "ast.h"
#include "lexer.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

Ir *init_ir(Parser *p, const char *file_path) {
  Ir *ir = malloc(sizeof(Ir));
  if (!ir) {
    fprintf(stderr, "FATAL: Failed to allocate ir context.\n");
    exit(EXIT_FAILURE);
  }

  ir->p = p;
  ir->temp_c = 0;
  ir->ir_head = NULL;
  ir->ir_tail = NULL;
  ir->module = p->l->file;
  ir->ir_arena = init_arena(1024 * sizeof(IrNode));

  ir->file = fopen(file_path, "w");
  if (!ir->file) {
    fprintf(stderr, "FATAL: Failed to open file: %s\n", file_path);
    free(ir);
    exit(EXIT_FAILURE);
  }

  return ir;
}

bool is_mem_op(irop_t op) {
  switch (op) {
  case OP_ALLOC:
  case OP_LOAD:
  case OP_STORE: return true;

  default: return false;
  }
}

const char *irop_to_str(irop_t op) {
  switch (op) {
  case OP_NEG: return "neg";
  case OP_ADD: return "add";
  case OP_SUB: return "sub";
  case OP_DIV: return "div";
  case OP_MUL: return "mul";
  case OP_MOD: return "mod";
  case OP_AND: return "and";
  case OP_OR: return "or";
  case OP_SHL: return "shl";
  case OP_SHR: return "shr";

  case OP_ALLOC: return "alloc";
  case OP_LOAD: return "load";
  case OP_STORE: return "store";

  case OP_EQ: return "eq";
  case OP_NE: return "ne";
  case OP_GE: return "ge";
  case OP_GT: return "gt";
  case OP_LE: return "le";
  case OP_LT: return "lt";

  default: return "?OP?";
  }
}

IrNode *new_ir_op(Arena *arena, size_t dest, irop_t op, size_t temp_1, size_t temp_2) {
  IrNode *node = arena_alloc(arena, sizeof(IrNode));
  *node = (IrNode){
      .kind = IR_OPERATION, .temp_dest = dest, .op = op, .temp_1 = temp_1, .temp_2 = temp_2};
  return node;
}

IrNode *new_ir_named(Arena *arena, IrKind kind, const char *name, size_t dest) {
  IrNode *node = arena_alloc(arena, sizeof(IrNode));
  *node = (IrNode){.kind = kind, .name = name, .temp_dest = dest};
  return node;
}

void add_ir_node(IrNode **t_node, IrNode *node) {
  if (!node) {
    return;
  }
  node->next = NULL;
  if (*t_node) {
    (*t_node)->next = node;
  }
  *t_node = node;
}

VarInfo *lookup_symbol(AstScope *scope, const char *name) {
  AstScope *curr_scope = scope;
  while (curr_scope != NULL) {
    if (curr_scope->symtab && has_in_hash_set(curr_scope->symtab, name)) {
      return (VarInfo *)get_from_hash_set(curr_scope->symtab, name);
    }
    curr_scope = curr_scope->parent;
  }
  return NULL;
}

size_t ir_expr_f(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  if (!node) {
    return 0;
  }

  if (node->kind == AST_ATOM) {

    switch (node->atom_n->kind) {

    case INT: {
      size_t temperory = ++(ir->temp_c);
      size_t _int = atoi(node->atom_n->value);
      add_ir_node(block_tail, new_ir_op(ir->ir_arena, temperory, OP_STORE, _int, 0));
      return temperory;
    }

      // case IDENTIFIER: {
      //   const char *var_name = node->atom_n->value;
      //   VarInfo *var = lookup_symbol(scope, var_name);

      //   if (!var) {
      //     fprintf(stderr, "FATAL: Undefined variable '%s'\n", var_name);
      //     exit(EXIT_FAILURE);
      //   }

      //   if (var->kind == AUTO_VAR || var->kind == PARAM_VAR) {
      //     fprintf(ir->file, "  mov rax, qword [rbp - %zu]\n", var->offset * 8);
      //     fprintf(ir->file, "  sub rsp, 8\n");
      //     fprintf(ir->file, "  mov qword [rsp], rax\n");
      //   } else if (var->kind == GLOBAL_VAR) {
      //     fprintf(ir->file, "  mov rax, qword [var_%s]\n", var_name);
      //     fprintf(ir->file, "  sub rsp, 8\n");
      //     fprintf(ir->file, "  mov qword [rsp], rax\n");
      //   }
      //   break;
      // }

    default:
      fprintf(stderr, "DEBUG atom value=%s kind=%d\n", node->atom_n->value, node->atom_n->kind);
      fprintf(stderr, "FATAL: Unhandled atom kind (%d) in ir_expr_f\n", (int)node->atom_n->kind);
      exit(EXIT_FAILURE);
    }

    return 0;
  }

  if (node->kind == AST_BINARY) {

    size_t temp_1 = ir_expr_f(ir, node->binary_n->left, scope, block_tail);
    size_t temp_2 = ir_expr_f(ir, node->binary_n->right, scope, block_tail);

    size_t temperory = ++(ir->temp_c);
    int op = (node->binary_n->op - NOT) + 1; // this is order dependent on TokenKind enum.
    // TokenKind and irop_t order needs to match!
    add_ir_node(block_tail, new_ir_op(ir->ir_arena, temperory, op, temp_1, temp_2));
    return temperory;
  }

  fprintf(stderr, "FATAL: Unhandled node kind (%d) in ir_expr_f\n", (int)node->kind);
  exit(EXIT_FAILURE);
}

IrNode *ir_auto_s(Ir *ir, AstScope *scope) {
  size_t temperory = ++(ir->temp_c);
  const char *var_name = ir->t_node->name_s;
  VarInfo *info = lookup_symbol(scope, var_name);
  info->temp_dest = temperory;
  return new_ir_op(ir->ir_arena, temperory, OP_ALLOC, 0, 0);
}

IrNode *ir_extrn_s(Ir *ir) { return new_ir_named(ir->ir_arena, IR_EXTRN, ir->t_node->name_s, 0); }

IrNode *ir_return_s(Ir *ir, AstScope *scope, IrNode **block_tail) {
  if (!ir->t_node) {
    fprintf(stderr, "FATAL: Invalid return statement\n");
    exit(EXIT_FAILURE);
  }

  ir_expr_f(ir, ir->t_node->node, scope, block_tail);
  size_t temperory = ir->temp_c;

  IrNode *return_node = arena_alloc(ir->ir_arena, sizeof(IrNode));

  if (!ir->t_node->node) {
    *return_node = (IrNode){.kind = IR_RETURN, .temp_dest = temperory};
    return return_node;
  }

  *return_node = (IrNode){.kind = IR_RETURN, .temp_dest = temperory};
  return return_node;
}

void ir_statements(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail);

void ir_scope(Ir *ir, AstNode *scope_node, IrNode **block_tail) {
  if (!scope_node || !scope_node->scope_n) {
    fprintf(stderr, "FATAL: no scope!\n");
    exit(EXIT_FAILURE);
  }

  AstScope *inner_scope = scope_node->scope_n;
  AstNode *curr = inner_scope->body;
  while (curr != NULL) {
    ir->t_node = curr;
    ir_statements(ir, curr, inner_scope, block_tail);
    curr = curr->next;
  }
}

void ir_statements(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  switch (curr->kind) {
  case AST_RETURN: add_ir_node(block_tail, ir_return_s(ir, scope, block_tail)); break;
  case AST_SCOPE: ir_scope(ir, curr, block_tail); break;
  case AST_AUTO: add_ir_node(block_tail, ir_auto_s(ir, scope)); break;
  case AST_EXTRN: add_ir_node(block_tail, ir_extrn_s(ir)); break;
  default:
    fprintf(stderr, "FATAL: Unhandled node kind (%d) in ir_statements\n", (int)curr->kind);
    exit(EXIT_FAILURE);
    break;
  }
}

IrNode *ir_function_s(Ir *ir) {
  const char *func_name = ir->t_node->function_n->name;
  size_t params = ir->t_node->function_n->params;

  AstNode *save_func = ir->t_node;
  AstNode *body_scope = save_func->function_n->body;

  IrNode *entry = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *entry = (IrNode){.kind = IR_LABEL, .name = "entry"};
  IrNode *block_tail = entry;

  ir_statements(ir, body_scope, ir->p->global_scope_n, &block_tail);

  IrNode *func_node = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *func_node = (IrNode){.kind = IR_FUNCTION, .name = func_name, .params = params, .nodes = entry};

  return func_node;
}

void gen_ir(Ir *ir) {
  IrNode *module = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *module = (IrNode){.kind = IR_MODULE, .name = ir->module};
  ir->ir_head = module;
  ir->ir_tail = module;

  AstNode *curr = ir->p->ast_head ? ir->p->ast_head->next : NULL;
  while (curr != NULL) {
    if (curr->kind == AST_FUNCTION) {
      ir->t_node = curr;
      add_ir_node(&ir->ir_tail, ir_function_s(ir));
    }
    curr = curr->next;
  }
}

void dump_ir(Ir *ir) {
  IrNode *curr = ir->ir_head;
  while (curr != NULL) {
    switch (curr->kind) {
    case IR_MODULE: fprintf(ir->file, "module \"%s\"\n\n", curr->name); break;

    case IR_FUNCTION:
      fprintf(ir->file, "func $%s() %zu {\n", curr->name, curr->params);
      IrNode *t_curr = curr->nodes;
      while (t_curr != NULL) {
        switch (t_curr->kind) {
        case IR_LABEL: fprintf(ir->file, "@%s\n", t_curr->name); break;
        case IR_OPERATION: {
          const char *op = irop_to_str(t_curr->op);
          if (is_mem_op(t_curr->op)) {
            fprintf(ir->file, "  %%t%zu = %s %zu\n", t_curr->temp_dest, op, t_curr->temp_1);
          } else {
            fprintf(ir->file, "  %%t%zu = %s %%t%zu, %%t%zu\n", t_curr->temp_dest, op,
                    t_curr->temp_1, t_curr->temp_2);
          }
          break;
        }
        case IR_EXTRN: fprintf(ir->file, "  extrn %s\n", t_curr->name); break;
        case IR_RETURN: fprintf(ir->file, "  ret %%t%zu\n", t_curr->temp_dest); break;
        default: break;
        }
        t_curr = t_curr->next;
      }
      fprintf(ir->file, "}\n");
      break;

    default: break;
    }
    curr = curr->next;
  }
}

void free_ir(Ir *ir) {
  if (ir->file) {
    fclose(ir->file);
  }
  free_arena(ir->ir_arena);
  free(ir);
}
