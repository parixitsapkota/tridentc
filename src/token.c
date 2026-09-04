#include "token.h"

char *token_kind_to_str(TokenKind kind) {
  switch (kind) {
  // Misc Tokens
  case UNKNOWN: return "UNKNOWN"; break;
  case END_OF_TOKEN: return "END_OF_TOKEN"; break;

  // Identifier & Literals
  case IDENTIFIER: return "IDENTIFIER"; break;
  case INT: return "INT"; break;
  case FLOAT: return "FLOAT"; break;
  case STRING: return "STRING"; break;
  case CHARACTER: return "CHARACTER"; break;
  case LABLE: return "LABLE"; break;

  // Keywords
  case EXTERN: return "extern"; break;
  case AUTO: return "auto"; break;
  case IF: return "if"; break;
  case ELSE: return "else"; break;
  case WHILE: return "while"; break;
  case RETURN: return "return"; break;

  // Seperator
  case O_BRACE: return "{"; break;
  case C_BRACE: return "}"; break;
  case O_PREN: return "("; break;
  case C_PREN: return ")"; break;
  case O_BRACKET: return "["; break;
  case C_BRACKET: return "]"; break;
  case SEMICOLON: return ";"; break;

  // Operator
  case COMMA: return ","; break;
  case DOT: return "."; break;
  case ADD: return "+"; break;
  case SUB: return "-"; break;
  case MUL: return "*"; break;
  case DEV: return "/"; break;
  case MOD: return "%%"; break;
  case ASSIGN: return "="; break;

  // Default
  default: return "nil"; break;
  }
}
