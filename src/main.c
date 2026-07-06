#include <stdio.h>
#include <stdlib.h>

#include "trident.h"

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "USAGE: %s <FILE>\n", argv[0]);
    return 1;
  }

  const char *file_path = argv[1];

  FILE *file = fopen(file_path, "rb");
  if (!file) {
    fprintf(stderr, "FATAL : Failed to open file: %s\n", file_path);
    return 1;
  }
  size_t buff_len = 0;
  char *buffer = read_file(file, &buff_len);
  fclose(file);

  Lexer *l = init_lexer(file_path, buffer, buff_len);
  lexer(l);

  Token *tok = l->tok_head->next;
  while (tok != NULL) {
    if (tok->lexeme) {
      printf("Kind: %-3d Lexeme: %s\n", tok->kind, tok->lexeme);
      free((char *)tok->lexeme);
    }
    tok = tok->next;
  }

  free_lexer(l);

  free(buffer);

  return 0;
}