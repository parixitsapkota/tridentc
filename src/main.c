#include <stdio.h>
#include <stdlib.h>

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

  system("mkdir -p asm/");
  const char *out_file_path = "asm/out.asm";

  Cgen *c = init_cgen(p, out_file_path);
  cgen(c);

  free_parser(p);
  free_lexer(l);
  free_cgen(c);

  char cmd[2048];

#if defined(__linux__) || defined(_TUX)
  sprintf(cmd, "nasm -f elf64 %s -o %s.o", out_file_path, out_file_path);
  system(cmd);
  sprintf(cmd, "ld -o a.out %s.o", out_file_path);

#elif defined(__MacOS__) || defined(_XOS)
  snprintf(cmd, sizeof(cmd), "nasm -f macho64 %s -o %s.o", out_file_path, out_file_path);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "clang - arch x86_64 %s.o -o a.out", out_file_path);

#elif defined(__FreeBSD__) || defined(_BSD)
  snprintf(cmd, sizeof(cmd), "nasm -f elf64 %s -o %s.o", out_file_path, out_file_path);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "ld -o a.out %s.o", out_file_path);

#elif defined(__Windows__) || defined(_WIN32)
  snprintf(cmd, sizeof(cmd), "nasm -f win64 %s -o %s.obj", out_file_path, out_file_path);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "gcc %s.obj -o a.exe", out_file_path);
#endif

  system(cmd);

  return 0;
}