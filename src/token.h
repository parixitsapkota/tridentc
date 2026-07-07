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

#endif // _TRIDENT_TOKEN_H_
