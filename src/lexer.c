#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keywords.h"
#include "trident.h"

// Lexer helper funcions
char peak(const Lexer *l, int offset);
void consume(Lexer *l);
void add_token(Lexer *l, Token t);
// P_Print helper function
char *token_kind_to_str(TokenKind kind);
// Lexer core functions
char *substr(const char *buffer, const size_t start, const size_t end);
char *get_word(Lexer *l);
char *get_string_ident(Lexer *l);
char *get_string(Lexer *l);
char *get_digit(Lexer *l, TokenKind *kind);

Lexer *init_lexer(const char *file, const char *buffer, size_t buf_len) {
  Lexer *l = malloc(sizeof(Lexer));
  l->file = file;
  l->buffer = buffer;
  l->buf_len = buf_len;
  l->i = 0;
  l->ln = 1;
  l->cn = 1;
  l->tokens = init_arena(sizeof(Token) * TOKENS_STORE);
  // Temp vars.
  l->t_token = NULL;
  l->tok_head = NULL;
  return l;
}

void lexer(Lexer *l) {
  l->tok_head = arena_alloc(l->tokens, sizeof(Token));
  l->t_token = l->tok_head;

  while (l->i < l->buf_len) {
    char c = peak(l, 0);
    l->t_cn = l->cn;

    // Count lines.
    if (isspace(c)) {
      if (c == '\n') {
        l->cn = 0;
        ++l->ln;
      } else {
        ++l->cn;
      }
      consume(l);
      continue;
    }

    // Collect Identifiers.
    if (isalpha(c) || c == '_') {
      size_t start = l->i;
      char *word = get_word(l);
      size_t word_len = l->i - start;

      const struct Keyword *k = get_keyword_kind(word, word_len);
      if (k != NULL) {
        add_token(l, (Token){k->token_kind, NULL, l->ln, l->t_cn, NULL});
        free(word);
      } else {
        add_token(l, (Token){IDENTIFIER, word, l->ln, l->t_cn, NULL});
      }
      continue;
    }

    // Handle digits.
    if (isdigit((unsigned char)c)) {
      TokenKind kind;
      char *num = get_digit(l, &kind);
      add_token(l, (Token){kind, num, l->ln, l->t_cn, NULL});
      continue;
    }

    // Collect Strings
    if (c == '"') {
      char *str = get_string(l);
      add_token(l, (Token){STRING, str, l->ln, l->t_cn, NULL});
      continue;
    }

    // Skip Comments.
    if (c == '/' && peak(l, 1) == '/') {
      while (l->i < l->buf_len && peak(l, 0) != '\n') {
        consume(l);
      }
      continue;
    }

    // Collect Directive & StringIdent.
    if (c == '@' && ((isalpha(peak(l, 1)) || peak(l, 1) == '"'))) {
      consume(l);
      if (peak(l, 0) == '"') {
        char *word = get_string_ident(l);
        add_token(l, (Token){IDENTIFIER, word, l->ln, l->t_cn, NULL});
      } else {
        char *lable = get_word(l);
        add_token(l, (Token){LABLE, lable, l->ln, l->t_cn, NULL});
      }
      continue;
    }

    {
      // Handle operators and separators.
      TokenKind kind;
      switch (c) {
      case '{': kind = O_BRACE; break;
      case '}': kind = C_BRACE; break;
      case '[': kind = O_BRACKET; break;
      case ']': kind = C_BRACKET; break;
      case '(': kind = O_PREN; break;
      case ')': kind = C_PREN; break;
      case ';': kind = SEMICOLON; break;
      case ',': kind = COMMA; break;
      case '.': kind = DOT; break;
      case '+': kind = ADD; break;
      case '-': kind = SUB; break;
      case '*': kind = MUL; break;
      case '/': kind = DEV; break;
      case '%': kind = MOD; break;
      case '=': kind = ASSIGN; break;
      default: kind = UNKNOWN;
      }
      if (kind == UNKNOWN) {
        add_token(l, (Token){UNKNOWN, NULL, l->ln, l->t_cn, NULL});
        consume(l);
        continue;
      }
      add_token(l, (Token){kind, NULL, l->ln, l->t_cn, NULL});
      consume(l);
      continue;
    }
  }
}

void free_lexer(Lexer *l) {
  Token *tok = l->tok_head->next;
  while (tok != NULL) {
    if (tok->lexeme) {
      free((char *)tok->lexeme);
    }
    tok = tok->next;
  }
  free_arena(l->tokens);
  free(l);
}

// Lexer helper funcions
char peak(const Lexer *l, const int offset) { return l->buffer[l->i + offset]; }

void consume(Lexer *l) {
  ++l->i;
  ++l->cn;
}

void add_token(Lexer *l, Token t) {
  Token *new_token = arena_alloc(l->tokens, sizeof(Token));
  *new_token = t;
  new_token->next = NULL;
  l->t_token->next = new_token;
  l->t_token = new_token;
}

// Lexer core functions
char *substr(const char *buffer, const size_t start, const size_t end) {
  const size_t length = end - start;
  char *substr = malloc(length + 1);
  strncpy(substr, buffer + start, length);
  substr[length] = '\0';
  return substr;
}

char *get_word(Lexer *l) {
  const size_t start = l->i;
  while (isalnum(peak(l, 0)) || peak(l, 0) == '_') {
    consume(l);
  }
  return substr(l->buffer, start, l->i);
}

char *get_string(Lexer *l) {
  consume(l); // Skip first char.
  const size_t start = l->i;

  while (peak(l, 0) != '"') {
    if (peak(l, 0) == '\0' || peak(l, 0) == '\n') {
      fprintf(stderr, "%s:%zu:%zu: Unterminated string.", l->file, l->ln, l->t_cn);
      exit(EXIT_FAILURE);
    }
    consume(l);
  }
  if (peak(l, 0) == '"') {
    consume(l); // Skip last char.
  }
  return substr(l->buffer, start, l->i - 1);
}

char *get_string_ident(Lexer *l) {
  consume(l); // Skip char `"`.
  const size_t start = l->i;

  while (peak(l, 0) != '"') {
    if (peak(l, 0) == '\0' || peak(l, 0) == '\n') {
      fprintf(stderr, "%s:%zu:%zu: Unterminated identifier string.", l->file, l->ln, l->t_cn);
      exit(EXIT_FAILURE);
    }
    consume(l);
  }
  if (peak(l, 0) == '"') {
    consume(l); // Skip char `"`.
  }
  return substr(l->buffer, start, l->i - 1);
}

static bool is_octal_digit(char c) { return c >= '0' && c <= '7'; }

static bool is_hex_digit(char c) { return isdigit((unsigned char)c) || (c >= 'A' && c <= 'F'); }

static char *get_binary(Lexer *l, TokenKind *kind) {
  size_t start = l->i;
  consume(l); // '0'
  consume(l); // 'b'
  while (peak(l, 0) == '0' || peak(l, 0) == '1') {
    consume(l);
  }
  *kind = INT;
  return substr(l->buffer, start, l->i);
}

static char *get_decimal(Lexer *l, TokenKind *kind) {
  size_t start = l->i;
  *kind = INT;
  while (isdigit((unsigned char)peak(l, 0))) {
    consume(l);
  }

  if (peak(l, 0) == '.') {
    *kind = FLOAT;
    consume(l); // consume '.'

    while (isdigit((unsigned char)peak(l, 0))) {
      consume(l);
    }

    if (peak(l, 0) == 'p') {
      consume(l); // 'p'
      if (peak(l, 0) == '+' || peak(l, 0) == '-') {
        consume(l);
      }
      while (is_octal_digit(peak(l, 0))) {
        consume(l);
      }
    }
  }
  return substr(l->buffer, start, l->i);
}

void get_prefix(Lexer *l, bool *has_dot, bool (*func)(char)) {
  consume(l); // '0'
  consume(l); // 'x`,`_'

  while (func(peak(l, 0))) {
    consume(l);
  }

  *has_dot = false;
  if (peak(l, 0) == '.') {
    *has_dot = true;
    consume(l);
    while (func(peak(l, 0))) {
      consume(l);
    }
  }
}

void get_suffix(Lexer *l, TokenKind *kind, bool has_dot, bool (*func)(char)) {
  if (peak(l, 0) == 'p') {
    *kind = FLOAT;
    consume(l); // 'p'
    if (peak(l, 0) == '+' || peak(l, 0) == '-') {
      consume(l);
    }
    while (func(peak(l, 0))) {
      consume(l);
    }
  } else if (has_dot) {
    *kind = FLOAT;
  } else {
    *kind = INT;
    if (peak(l, 0) == 'S') {
      consume(l);
    }
  }
}

static char *get_num(Lexer *l, TokenKind *kind, bool (*func)(char)) {
  size_t start = l->i;

  bool has_dot = false;
  get_prefix(l, &has_dot, func);

  get_suffix(l, kind, has_dot, func);
  return substr(l->buffer, start, l->i);
}

// TODO: report error on malformed digits.
char *get_digit(Lexer *l, TokenKind *kind) {
  if (peak(l, 0) == '0' && peak(l, 1) == 'b') {
    return get_binary(l, kind);
  }
  if (peak(l, 0) == '0' && peak(l, 1) == '_') {
    return get_num(l, kind, is_octal_digit);
  }
  if (peak(l, 0) == '0' && peak(l, 1) == 'x') {
    return get_num(l, kind, is_hex_digit);
  }
  return get_decimal(l, kind);
}
