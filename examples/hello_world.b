char(str, i) {
    return *(str + i);
}

puts(str) {
  extrn putchar;
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
