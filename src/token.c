#include "token.h"

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

Position position(size_t ln, size_t cn) { return (Position){.ln = ln, .cn = cn}; }
