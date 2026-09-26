puts(str) {
  extrn putchar, char;
  auto i;
  i = 0;
  while (char(str, i)) {
    putchar(char(str, i));
    ++i;
  }
}

main() {
  puts("Hello, world!*n");
}
