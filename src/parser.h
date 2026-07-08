#ifndef _TRIDENT_PARSER_H_
#define _TRIDENT_PARSER_H_

#include "ast.h"
#include "trident.h"

Token *ppeak(const Parser *p);
Token *curr(const Parser *p);
Token *pconsume(Parser *p);
void expect(Parser *p, TokenKind kind);

AstNode *parse_atom_f(Parser *p);
AstNode *parse_expr_f(Parser *p, Precedence prec);

#endif // _TRIDENT_PARSER_H_
