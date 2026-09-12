#include <stdio.h>
#include <stdlib.h>

#include "ap.h"
#include "cgen.h"
#include "lexer.h"
#include "parser.h"
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
  free(buffer);

  Parser *p = init_parser(l);
  parser(p);

  char out_file_path[1024];
  sprintf(out_file_path, "%s.asm", file_path);

  Cgen *c = init_cgen(p, out_file_path);

  cgen(c);

  // print_ast(p);

  free_parser(p);
  free_lexer(l);
  free_cgen(c);
  return 0;
}