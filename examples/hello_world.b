puts(str) {
  extrn putchar;
  while (str) {
    putchar(str);
    ++str;
  }
}

main() {
  puts("Hello, world!*n");
}
