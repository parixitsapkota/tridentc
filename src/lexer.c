#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dep/keywords.h"
#include "lexer.h"

// Lexer helper funcions
char peak(const Lexer *l, int offset);
void consume(Lexer *l);
void add_token(Lexer *l, TokenKind kind, const char *lexeme, Position position);
Position position(size_t ln, size_t cn);
// P_Print helper function
char *token_kind_to_str(TokenKind kind);
// Lexer core functions
char *substr(const char *buffer, const size_t start, const size_t end);
char *get_word(Lexer *l);
char *get_string_ident(Lexer *l);
char *get_string(Lexer *l);
char *get_digit(Lexer *l);

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
  l->tok_head->next = NULL;
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
        add_token(l, k->token_kind, NULL, position(l->ln, l->t_cn));
        free(word);
      } else {
        add_token(l, IDENTIFIER, word, position(l->ln, l->t_cn));
      }
      continue;
    }

    // Handle digits.
    if (isdigit((unsigned char)c)) {
      char *num = get_digit(l);
      add_token(l, INT, num, position(l->ln, l->t_cn));
      continue;
    }

    // Collect Strings
    if (c == '"') {
      char *str = get_string(l);
      add_token(l, STRING, str, position(l->ln, l->t_cn));
      continue;
    }

    // Skip Comments.
    if (c == '/' && peak(l, 1) == '*') {
      while (l->i < l->buf_len) {
        if (peak(l, 0) == '*' && peak(l, 1) == '/') {
          consume(l);
          consume(l);
          break;
        }
        if (peak(l, 0) == '\n') {
          l->cn = 0;
          ++l->ln;
        }
        consume(l);
      }
      continue;
    }

#define CASE_1(a, out_a)                                                                         \
  case a: kind = out_a; break

#define CASE_2(a, b, out_a, out_a_b)                                                             \
  case a:                                                                                        \
    if (peak(l, 1) == (b)) {                                                                     \
      kind = (out_a_b);                                                                          \
      consume(l);                                                                                \
    } else {                                                                                     \
      kind = (out_a);                                                                            \
    }                                                                                            \
    break

    {
      TokenKind kind;

      switch (c) {
        CASE_1('{', O_BRACE);
        CASE_1('}', C_BRACE);
        CASE_1('[', O_BRACKET);
        CASE_1(']', C_BRACKET);
        CASE_1('(', O_PREN);
        CASE_1(')', C_PREN);
        CASE_1(';', SEMICOLON);
        CASE_1(':', COLON);
        CASE_1('?', Q_MARK);
        CASE_1('&', BIT_AND);
        CASE_1('|', BIT_OR);

        CASE_1(',', COMMA);
        CASE_1('*', MUL);
        CASE_1('/', DEV);
        CASE_1('%', MOD);

        CASE_2('+', '+', ADD, INC);
        CASE_2('-', '-', SUB, DEC);
        CASE_2('!', '=', NOT, NOT_EQUAL);
        CASE_2('=', '=', ASSIGN, EQUAL);

      case '<':
        if (peak(l, 1) == '=') {
          kind = LESSER_EQUAL;
          consume(l);
        } else if (peak(l, 1) == '<') {
          kind = BITSHIFT_L;
          consume(l);
        } else {
          kind = LESSER;
        }
        break;

      case '>':
        if (peak(l, 1) == '=') {
          kind = GREATER_EQUAL;
          consume(l);
        } else if (peak(l, 1) == '>') {
          kind = BITSHIFT_R;
          consume(l);
        } else {
          kind = GREATER;
        }
        break;

      default: kind = UNKNOWN; break;
      }

      if (kind == UNKNOWN) {
        add_token(l, UNKNOWN, NULL, position(l->ln, l->t_cn));
        consume(l);
        continue;
      }
      add_token(l, kind, NULL, position(l->ln, l->t_cn));
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

void add_token(Lexer *l, TokenKind kind, const char *lexeme, Position position) {
  Token *new_token = arena_alloc(l->tokens, sizeof(Token));
  *new_token = (Token){.kind = kind, .lexeme = lexeme, .position = position, .next = NULL};
  new_token->next = NULL;
  l->t_token->next = new_token;
  l->t_token = new_token;
}

Position position(size_t ln, size_t cn) { return (Position){.ln = ln, .cn = cn}; }

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

char *get_digit(Lexer *l) {
  const size_t start = l->i;
  while (isdigit(peak(l, 0))) {
    consume(l);
  }
  return substr(l->buffer, start, l->i);
}

char *token_kind_to_str(TokenKind kind) {
  switch (kind) {
  case UNKNOWN: return "UNKNOWN";
  case END_OF_TOKEN: return "END_OF_TOKEN";
  case IDENTIFIER: return "IDENTIFIER";
  case INT: return "INT";
  case STRING: return "STRING";
  case CHARACTER: return "CHARACTER";
  case LABLE: return "LABLE";
  case EXTRN: return "EXTRN";
  case AUTO: return "AUTO";
  case IF: return "IF";
  case ELSE: return "ELSE";
  case WHILE: return "WHILE";
  case GOTO: return "GOTO";
  case RETURN: return "RETURN";
  case O_BRACE: return "{";
  case C_BRACE: return "}";
  case O_PREN: return "(";
  case C_PREN: return ")";
  case O_BRACKET: return "[";
  case C_BRACKET: return "]";
  case SEMICOLON: return ";";
  case COLON: return ":";
  case Q_MARK: return "?";
  case NOT: return "!";
  case COMMA: return ",";
  case ADD: return "+";
  case SUB: return "-";
  case MUL: return "*";
  case DEV: return "/";
  case MOD: return "%";
  case ASSIGN: return "=";
  case INC: return "++";
  case DEC: return "--";
  case BIT_AND: return "&";
  case BIT_OR: return "|";
  case BITSHIFT_L: return "<<";
  case BITSHIFT_R: return ">>";
  case EQUAL: return "==";
  case NOT_EQUAL: return "!=";
  case LESSER: return "<";
  case GREATER: return ">";
  case LESSER_EQUAL: return "<=";
  case GREATER_EQUAL: return ">=";

  // Default
  default: return "nil";
  }
}
