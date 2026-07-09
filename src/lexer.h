#ifndef _TRIDENT_LEXER_H_
#define _TRIDENT_LEXER_H_

#include "ast.h"
#include "token.h"

// Lexer Structure
typedef struct {
  // buffer file name
  const char *file;
  // Input buffer
  const char *buffer;
  size_t buf_len;
  // Position
  size_t i;  // index
  size_t ln; // line number
  size_t cn; // colume number
  // Token List
  Arena *tokens;
  Token *tok_head;
  // Helper/Temp vars
  Token *t_token;
  size_t t_cn;
} Lexer;

#define TOKENS_STORE 1024

/// Returns a lexer context based on given buffer and length of the buffer.
Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len);
/// Lexes based on the given lexer context and mutates the state accordingly.
void lexer(Lexer *l);
/// Frees the allocated memory in the lexing context.
void free_lexer(Lexer *l);

#endif // _TRIDENT_LEXER_H_
