#ifndef _TRIDENT_H_
#define _TRIDENT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/// Reads the given file and returns it as a `char *`.
/// also sets bytes to number of bytes red if not `NULL`.
char *read_file(FILE *file, size_t *bytes);

// Arena
typedef struct __arena_region__ {
  size_t cap;                    /* : Region capacity in bytes */
  size_t offset;                 /* : Current bytes used in region */
  struct __arena_region__ *next; /* : Next region in chain */
  uint8_t *bytes;                /* : Actual memory region bytes */
} __arena_region__;

typedef struct __arena__ {
  size_t region_size; /* : Default size in bytes for new regions */
  __arena_region__ *begin;
  __arena_region__ *end;
} __arena__;

#define Arena __arena__

/// Initializes a new __arena_region__ with the given byte capacity.
__arena_region__ *init_arena_region(size_t cap);

/// Initializes a new __arena__ with the given default region size.
__arena__ *init_arena(size_t region_size);

/// Allocates a new region, chains it after the arena's current end, and
/// advances the arena's end pointer to it. Capacity is at least
/// `min_cap` bytes, or the arena's default region_size, whichever is larger.
int push_new_arena_region(__arena__ *arena, size_t min_cap);

/// Allocates `size` bytes from the arena. Walks forward through already
/// chained regions before allocating a new one.
void *arena_alloc(__arena__ *arena, size_t size);

/// Copies `size` bytes from `data` into a fresh allocation in the arena.
void *arena_memdup(__arena__ *arena, void *data, size_t size);

/// Resets all regions' offsets to 0 without freeing them, so the arena's
/// memory chain is fully reusable.
void arena_reset(__arena__ *arena);

/// Frees the entire region chain and the arena itself.
void free_arena(__arena__ *arena);

// Token Kinds
typedef enum {
  // Misc Tokens
  UNKNOWN = 0,
  END_OF_TOKEN,

  // Identifier & Literals
  IDENTIFIER,
  INT,
  FLOAT,
  STRING,
  CHARACTER,
  LABLE,

  // Keywords
  RETURN,
  FUNCTION,
  PUBLIC,

  // Seperator
  O_BRACE,   // `{`
  C_BRACE,   // `}`
  O_PREN,    // `(`
  C_PREN,    // `)`
  O_BRACKET, // `[`
  C_BRACKET, // `]`
  SEMICOLON, // `;`
  // Operator
  COMMA,  // `,`
  DOT,    // `.`
  ADD,    // `+`
  SUB,    // `-`
  MUL,    // `*`
  DEV,    // `/`
  MOD,    // `%`
  ASSIGN, // `=`

} TokenKind;

// Token Defination
typedef struct Token {
  // Value
  TokenKind kind;
  const char *lexeme;
  // Position
  size_t ln; // line number;
  size_t cn; // comume number;
  // Next Token
  struct Token *next;
} Token;

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
  // String store.
  Arena *string;
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

#endif // _TRIDENT_H_