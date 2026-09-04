![Banner](./res/trident.png)
An compiler for the "b" language.

## About B
B is a computer language intended for recursive, primarily non-numeric applications typified by system programming. B has a small, unrestrictive syntax that is easy to compile. Because of the unusual freedom of expression and a rich set of operators, B programs are often quite compact. 

## Build & Run
- Build Requirements : `clang-format clang make gperf nasm`
```bash
make MODE=release
```
- Run
```bash
./trident <FILE>
```

# Refrences
- [Users' Reference to B ](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/kbman.html)
- [Parse tree wiki ](https://en.wikipedia.org/wiki/Parse_tree)
- [Parser and AST : LLVM](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html)
- [x86_64 assembly syntax](https://www.cs.virginia.edu/~evans/cs216/guides/x86.html)
- [Linux Syscalls table](https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md)

# License
This project is License under the Apache-2.0 [License](https://github.com/parixitsapkota/tridentc?tab=Apache-2.0-1-ov-file)
