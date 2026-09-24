#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "ir.h"
#include "lexer.h"

_Noreturn void ir_fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("FATAL: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(EXIT_FAILURE);
}

Ir *init_ir(Parser *p, const char *file_path) {
  Ir *ir = calloc(1, sizeof(Ir));
  if (!ir) {
    fprintf(stderr, "FATAL: Failed to allocate ir context.\n");
    exit(EXIT_FAILURE);
  }

  ir->p = p;
  ir->module = p->l->file;
  ir->ir_arena = init_arena(1024 * sizeof(IrNode));

  ir->file = fopen(file_path, "w");
  if (!ir->file) {
    fprintf(stderr, "FATAL: Failed to open file: %s\n", file_path);
    free_arena(ir->ir_arena);
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

  case OP_EQ: return "eq";
  case OP_NE: return "ne";
  case OP_GE: return "ge";
  case OP_GT: return "gt";
  case OP_LE: return "le";
  case OP_LT: return "lt";

  case OP_ALLOC: return "alloc";
  case OP_ADDRESS: return "address";
  case OP_LOAD: return "load";
  case OP_STORE: return "store";

  case OP_CONST: return "const";

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

size_t emit_const(Ir *ir, IrNode **tail, long long value) {
  size_t t = ++ir->temp_c;
  IrNode *n = new_ir_op(ir->ir_arena, t, OP_CONST, 0, 0);
  n->imm = value;
  add_ir_node(tail, n);
  return t;
}

size_t emit_op(Ir *ir, IrNode **tail, irop_t op, size_t a, size_t b) {
  size_t t = ++ir->temp_c;
  add_ir_node(tail, new_ir_op(ir->ir_arena, t, op, a, b));
  return t;
}

void emit_store(Ir *ir, IrNode **tail, size_t value, size_t addr) {
  add_ir_node(tail, new_ir_op(ir->ir_arena, 0, OP_STORE, addr, value));
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

VarInfo *lookup_or_die(AstScope *scope, const char *name) {
  VarInfo *var = lookup_symbol(scope, name);
  if (!var) {
    ir_fatal("Undefined variable '%s'", name);
  }
  return var;
}

irop_t binop_to_irop(int tok) {
  switch (tok) {
  case ADD: return OP_ADD;
  case SUB: return OP_SUB;
  case MUL: return OP_MUL;
  case DEV: return OP_DIV;
  case MOD: return OP_MOD;
  case BIT_AND: return OP_AND;
  case BIT_OR: return OP_OR;
  case BITSHIFT_L: return OP_SHL;
  case BITSHIFT_R: return OP_SHR;
  case EQUAL: return OP_EQ;
  case NOT_EQUAL: return OP_NE;
  case GREATER: return OP_GT;
  case GREATER_EQUAL: return OP_GE;
  case LESSER: return OP_LT;
  case LESSER_EQUAL: return OP_LE;
  default: return 0;
  }
}

size_t ir_expr_f(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail);

size_t ir_lvalue(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  if (!node) {
    ir_fatal("Missing lvalue");
  }

  if (node->kind == AST_ATOM && node->atom_n->kind == IDENTIFIER) {
    const char *name = node->atom_n->value;
    VarInfo *var = lookup_or_die(scope, name);
    if (var->kind == AUTO_VAR || var->kind == PARAM_VAR) {
      if (var->temp_dest == 0) {
        ir_fatal("Variable '%s' has no storage slot yet", name);
      }
      return var->temp_dest;
    }
    ir_fatal("Global variable '%s': not supported by the IR yet", name);
  }

  if (node->kind == AST_UNARY && node->unary_n->op == MUL) {
    return ir_expr_f(ir, node->unary_n->node, scope, block_tail);
  }

  ir_fatal("Expression is not an lvalue");
}

size_t ir_expr_f(Ir *ir, AstNode *node, AstScope *scope, IrNode **block_tail) {
  if (!node) {
    return 0;
  }

  switch (node->kind) {
  case AST_ATOM: {
    switch (node->atom_n->kind) {
    case INT: return emit_const(ir, block_tail, strtoll(node->atom_n->value, NULL, 10));

    case IDENTIFIER: {
      const char *name = node->atom_n->value;
      VarInfo *var = lookup_or_die(scope, name);
      if (var->kind == AUTO_VAR || var->kind == PARAM_VAR) {
        if (var->temp_dest == 0) {
          ir_fatal("Variable '%s' has no storage slot yet", name);
        }
        return emit_op(ir, block_tail, OP_LOAD, var->temp_dest, 0);
      }
      ir_fatal("Global variable '%s': not supported by the IR yet", name);
    }

    default:
      ir_fatal("Unhandled atom kind (%d, value=%s) in ir_expr_f", (int)node->atom_n->kind,
               node->atom_n->value);
    }
  }

  case AST_UNARY: {
    int op = node->unary_n->op;
    AstNode *operand = node->unary_n->node;

    switch (op) {
    case BIT_AND: {
      size_t addr = ir_lvalue(ir, operand, scope, block_tail);
      return emit_op(ir, block_tail, OP_ADDRESS, addr, 0);
    }

    case MUL: {
      size_t addr = ir_expr_f(ir, operand, scope, block_tail);
      return emit_op(ir, block_tail, OP_LOAD, addr, 0);
    }

    case SUB: {
      size_t v = ir_expr_f(ir, operand, scope, block_tail);
      return emit_op(ir, block_tail, OP_NEG, v, 0);
    }

    case NOT: {
      size_t v = ir_expr_f(ir, operand, scope, block_tail);
      size_t zero = emit_const(ir, block_tail, 0);
      return emit_op(ir, block_tail, OP_EQ, v, zero);
    }

    case INC:
    case DEC: {
      size_t addr = ir_lvalue(ir, operand, scope, block_tail);
      size_t old = emit_op(ir, block_tail, OP_LOAD, addr, 0);
      size_t one = emit_const(ir, block_tail, 1);
      size_t nv = emit_op(ir, block_tail, op == INC ? OP_ADD : OP_SUB, old, one);
      emit_store(ir, block_tail, nv, addr);
      return nv;
    }

    default: ir_fatal("Unhandled unary op (%d) in ir_expr_f", op);
    }
  }

  case AST_BINARY: {
    int op = node->binary_n->op;

    if (op == ASSIGN) {
      size_t val = ir_expr_f(ir, node->binary_n->right, scope, block_tail);
      size_t addr = ir_lvalue(ir, node->binary_n->left, scope, block_tail);
      emit_store(ir, block_tail, val, addr);
      return val;
    }

    irop_t irop = binop_to_irop(op);
    if (!irop) {
      ir_fatal("Unhandled binary op (%d) in ir_expr_f", op);
    }
    size_t a = ir_expr_f(ir, node->binary_n->left, scope, block_tail);
    size_t b = ir_expr_f(ir, node->binary_n->right, scope, block_tail);
    return emit_op(ir, block_tail, irop, a, b);
  }

  default: ir_fatal("Unhandled node kind (%d) in ir_expr_f", (int)node->kind);
  }
}

void ir_auto_s(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  VarInfo *info = lookup_symbol(scope, curr->name_s);
  if (!info) {
    ir_fatal("Symbol '%s' not found in scope", curr->name_s);
  }
  info->temp_dest = emit_op(ir, block_tail, OP_ALLOC, 0, 0);
}

void ir_return_s(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  size_t ret_temp = ir_expr_f(ir, curr->node, scope, block_tail);
  add_ir_node(block_tail, new_ir_named(ir->ir_arena, IR_RETURN, NULL, ret_temp));
}

void ir_statements(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail);

void ir_scope(Ir *ir, AstNode *scope_node, IrNode **block_tail) {
  if (!scope_node || !scope_node->scope_n) {
    ir_fatal("no scope!");
  }

  AstScope *inner_scope = scope_node->scope_n;
  for (AstNode *curr = inner_scope->body; curr != NULL; curr = curr->next) {
    ir->t_node = curr;
    ir_statements(ir, curr, inner_scope, block_tail);
  }

  if (inner_scope->symtab) {
    free_hash_set(inner_scope->symtab);
  }
}

void ir_statements(Ir *ir, AstNode *curr, AstScope *scope, IrNode **block_tail) {
  Arena *arena = ir->ir_arena;
  switch (curr->kind) {
  case AST_RETURN: ir_return_s(ir, curr, scope, block_tail); break;
  case AST_SCOPE: ir_scope(ir, curr, block_tail); break;
  case AST_AUTO: ir_auto_s(ir, curr, scope, block_tail); break;
  case AST_EXTRN: add_ir_node(block_tail, new_ir_named(arena, IR_EXTRN, curr->name_s, 0)); break;
  case AST_EXPR: ir_expr_f(ir, curr->node, scope, block_tail); break;
  case AST_LABLE: add_ir_node(block_tail, new_ir_named(arena, IR_LABEL, curr->name_s, 0)); break;
  case AST_GOTO: add_ir_node(block_tail, new_ir_named(arena, IR_JUMP, curr->name_s, 0)); break;
  default: ir_fatal("Unhandled node kind (%d) in ir_statements", (int)curr->kind);
  }
}

IrNode *ir_function_s(Ir *ir, AstNode *func) {
  const char *func_name = func->function_n->name;
  size_t params = func->function_n->params;
  AstNode *body_scope = func->function_n->body;

  ir->temp_c = 0;

  IrNode *entry = arena_alloc(ir->ir_arena, sizeof(IrNode));
  *entry = (IrNode){.kind = IR_LABEL, .name = "entry"};
  IrNode *block_tail = entry;

  ir_scope(ir, body_scope, &block_tail);

  if (block_tail->kind != IR_RETURN) {
    size_t zero = emit_const(ir, &block_tail, 0);
    add_ir_node(&block_tail, new_ir_named(ir->ir_arena, IR_RETURN, NULL, zero));
  }

  free_hash_set(func->function_n->params_tab);

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
      add_ir_node(&ir->ir_tail, ir_function_s(ir, curr));
    }
    curr = curr->next;
  }
}

void dump_op(FILE *f, const IrNode *n) {
  const char *op = irop_to_str(n->op);
  switch (n->op) {
  case OP_CONST: fprintf(f, "  %%t%zu = %s %zu\n", n->temp_dest, op, n->imm); break;
  case OP_ALLOC: fprintf(f, "  %%t%zu = %s\n", n->temp_dest, op); break;
  case OP_LOAD:
  case OP_ADDRESS:
  case OP_NEG: fprintf(f, "  %%t%zu = %s %%t%zu\n", n->temp_dest, op, n->temp_1); break;
  case OP_STORE: fprintf(f, "  %s %%t%zu, %%t%zu\n", op, n->temp_1, n->temp_2); break;
  default:
    fprintf(f, "  %%t%zu = %s %%t%zu, %%t%zu\n", n->temp_dest, op, n->temp_1, n->temp_2);
    break;
  }
}

void dump_ir(Ir *ir, FILE *f) {
  for (IrNode *curr = ir->ir_head; curr != NULL; curr = curr->next) {
    switch (curr->kind) {
    case IR_MODULE: fprintf(f, "module \"%s\"\n\n", curr->name); break;

    case IR_FUNCTION: {
      fprintf(f, "func $%s() %zu {\n", curr->name, curr->params);
      for (IrNode *t = curr->nodes; t != NULL; t = t->next) {
        switch (t->kind) {
        case IR_LABEL: fprintf(f, "@%s\n", t->name); break;
        case IR_OPERATION: dump_op(f, t); break;
        case IR_EXTRN: fprintf(f, "  extrn %s\n", t->name); break;
        case IR_JUMP: fprintf(f, "  jmp @%s\n", t->name); break;
        case IR_RETURN:
          if (t->temp_dest) {
            fprintf(f, "  ret %%t%zu\n", t->temp_dest);
          } else {
            fprintf(f, "  ret\n");
          }
          break;
        case IR_MODULE:
        case IR_FUNCTION:
        case IR_CALL:
        case IR_BRANCH: break;
        }
      }
      fprintf(f, "}\n");
      break;
    }

    default: break;
    }
  }
}

void free_ir(Ir *ir) {
  if (ir->file) {
    fclose(ir->file);
  }
  free_arena(ir->ir_arena);
  free(ir);
}
