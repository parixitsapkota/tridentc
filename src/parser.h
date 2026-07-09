#ifndef _TRIDENT_PARSER_H_
#define _TRIDENT_PARSER_H_

#include "ast.h"
#include "lexer.h"
#include "trident.h"

// Parser Structure
typedef struct {
  Lexer *l;
  // Position
  size_t i; // index
  // Ast store
  Arena *ast;
  AstNode *ast_head;
  // Helper/Temp vars
  Token *tok;
  AstNode *t_node;
} Parser;

/// Returns a parser context based on given lexing context.
Parser *init_parser(Lexer *l);
/// parses based on the given lexer context and mutates the parser state accordingly.
void parser(Parser *p);
/// Frees the allocated memory in the Parsing context.
void free_parser(Parser *p);

typedef enum {
  PREC_UNKNOWN = -1,
  PREC_NONE = 0,
  PREC_COMMA,
  PREC_ASSIGNMENT,
  PREC_RANGE,
  PREC_OR,
  PREC_XOR,
  PREC_AND,
  PREC_BIT_OR,
  PREC_BIT_XOR,
  PREC_BIT_AND,
  PREC_RELATIVE,
  PREC_COMPARITIVE,
  PREC_BITSHIFT,
  PREC_ADDITIVE,
  PREC_MULTIPLICATIVE,
} Precedence;

Token *ppeak(const Parser *p);
Token *curr(const Parser *p);
Token *pconsume(Parser *p);
void expect(Parser *p, TokenKind kind);

AstNode *parse_atom_f(Parser *p);
AstNode *parse_expr_f(Parser *p, Precedence prec);

#endif // _TRIDENT_PARSER_H_
