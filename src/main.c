#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SHI_STRIP_PREFIX
#include "dep/shi_file.h"
#include "dep/shi_flags.h"

#include "cgen.h"
#include "info.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  bool *help = shi_flag_bool("-help", false, "show this output.");
  shi_flag_set_short(help, "h");

  bool *version = shi_flag_bool("-version", false, "show version information.");
  shi_flag_set_short(version, "v");

  char **name = shi_flag_str("-input", NULL, "input file name.");
  shi_flag_set_short(name, "i");

  char **output = shi_flag_str("-output", "out.asm", "output file name.");
  shi_flag_set_short(output, "o");

  bool *_start = shi_flag_bool("-add-brt", false, "compile with b-runtime.");
  shi_flag_set_short(_start, "r");

  if (!shi_flag_parse(argc, argv)) {
    shi_flag_print_error(stderr);
    fprintf(stderr, "Usage: %s [OPTIONS]\n", shi_flag_program_name());
    shi_flag_print_options(stderr);
    return 1;
  }

  if (*help) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", shi_flag_program_name());
    shi_flag_print_options(stderr);
    return 1;
  }

  if (*version) {
    fprintf(stderr, BOLD "Trident " VERSION_INFO RESET "\n");
    fprintf(stderr, DIM "Compiler:   " RESET CC_INFO "\n");
    fprintf(stderr, DIM "Build Time: " RESET TIME_INFO "\n");
    return 0;
  }

  if (!*name) {
    fprintf(stderr, "%s : No input file provided\n", shi_flag_program_name());
    fprintf(stderr, "Usage: %s -i <input.b>\n", shi_flag_program_name());
    return 1;
  }

  const char *file_path = *name;

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

  Cgen *c = init_cgen(p, *output);

  if (*_start) {
    cgen(c, true);
  } else {
    cgen(c, false);
  }

  free_lexer(l);
  free_parser(p);
  free_cgen(c);
  return 0;
}

#define SHI_ARENA_IMPLEMENTATION
#include "dep/shi_arena.h"
#define SHI_FILE_IMPLEMENTATION
#include "dep/shi_file.h"
#define SHI_FLAGS_IMPLEMENTATION
#include "dep/shi_flags.h"
#define SHI_HS_IMPLEMENTATION
#include "dep/shi_hs.h"
