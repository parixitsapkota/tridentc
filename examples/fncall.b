ret_val;
one, two;

foo() {
  ret_val = one + two;
  return 0;
}

main() {
  one = 1;
  two = 2;
  foo();
  return ret_val;
}
