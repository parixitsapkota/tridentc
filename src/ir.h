#ifndef _TRIDENT_IR_H_
#define _TRIDENT_IR_H_

#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "parser.h"

typedef struct IrNode IrNode;

typedef enum {
  // ArithmeticsOP
  OP_NEG = 1,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_AND,
  OP_OR,
  OP_SHL,
  OP_SHR,
  // ComparisonsOP
  OP_EQ,
  OP_NE,
  OP_GT,
  OP_GE,
  OP_LT,
  OP_LE,
  // MemoryOP
  OP_ALLOC,
  OP_LOAD,
  OP_STORE,
} irop_t;

typedef enum {
  IR_MODULE,
  IR_FUNCTION,
  IR_LABEL,
  IR_OPERATION,
  IR_EXTRN,
  IR_JUMP,
  IR_BRANCH,
  IR_CALL,
  IR_RETURN,
} IrKind;

struct IrNode {
  IrKind kind;

  const char *name;
  const char *val;
  union {
    size_t params;
    size_t temp_dest;
  };
  irop_t op;
  size_t temp_1;
  size_t temp_2;

  IrNode *nodes;
  IrNode *next;
};

// Ir context Structure
typedef struct {
  Parser *p;
  const char *module;
  FILE *file;
  size_t temp_c;
  IrNode *ir_head;
  IrNode *ir_tail;
  IrNode *t_ir_tail;
  Arena *ir_arena;
  // Helper/Temp vars
  AstNode *t_node;
} Ir;

Ir *init_ir(Parser *p, const char *file_path);
void free_ir(Ir *ir);
void gen_ir(Ir *ir);
void dump_ir(Ir *ir);

bool is_mem_op(irop_t op);
const char *irop_to_str(irop_t op);
IrNode *new_ir_op(Arena *arena, size_t dest, irop_t op, size_t temp_1, size_t temp_2);
IrNode *new_ir_named(Arena *arena, IrKind kind, const char *name, size_t dest);
void add_ir_node(IrNode **t_node, IrNode *node);

#endif // _TRIDENT_IR_H_
