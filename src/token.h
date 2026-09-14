#ifndef _TRIDENT_TOKEN_H_
#define _TRIDENT_TOKEN_H_

#include <stddef.h>

// Token Kinds
typedef enum {
  // Misc Tokens
  UNKNOWN = 0,
  END_OF_TOKEN,

  // Identifier & Literals
  IDENTIFIER,
  INT,
  STRING,
  CHARACTER,
  LABLE,

  // Keywords
  EXTERN,
  AUTO,
  IF,
  ELSE,
  WHILE,
  GOTO,
  RETURN,

  // Seperator
  O_BRACE,   // `{`
  C_BRACE,   // `}`
  O_PREN,    // `(`
  C_PREN,    // `)`
  O_BRACKET, // `[`
  C_BRACKET, // `]`
  SEMICOLON, // `;`
  COLON,     // `:`
  Q_MARK,    // `?`

  // Operator
  NOT,    // `!`
  COMMA,  // `,`
  ADD,    // `+`
  SUB,    // `-`
  MUL,    // `*`
  DEV,    // `/`
  MOD,    // `%`
  ASSIGN, // `=`

  INC, // `++`
  DEC, // `--`

  BIT_AND, // `&`
  BIT_OR,  // `|`

  BITSHIFT_L, // `<<`
  BITSHIFT_R, // `>>`

  EQUAL,         // `==`
  NOT_EQUAL,     // `!=`
  LESSER,        // `<`
  GREATER,       // `>`
  LESSER_EQUAL,  // `<=`
  GREATER_EQUAL, // `>=`
} TokenKind;

typedef struct {
  // Position
  size_t ln; // line number;
  size_t cn; // comume number;
} Position;

// Token Defination
typedef struct Token {
  // Value
  TokenKind kind;
  const char *lexeme;
  // Position
  Position position;
  // Next Token
  struct Token *next;
} Token;

/// Returns a tokenKind atring based on given tokenKind.
char *token_kind_to_str(TokenKind kind);
Position position(size_t ln, size_t cn);

#endif // _TRIDENT_TOKEN_H_