![Banner](./res/trident.png)

An compiler for the **B programming language**, targeting `x86_64` assembly.

---

## 📌 About the B Language

**B** is a typeless systems programming language developed by Ken Thompson and Dennis Ritchie at Bell Labs—the direct predecessor to **C**.

* **Designed for Systems:** Ideal for non-numeric applications, language processors, and operating systems.
* **Minimalist Syntax:** Simple grammar that allows fast, lightweight compilation.
* **Expressive & Compact:** Rich operator set and unrestrictive expressions lead to concise code.

---

## 🛠️ Requirements & Build Guide

### Prerequisites

| Type | Tools |
| --- | --- |
| **Required** | `clang`, `make`, `nasm` |
| **Optional** | `clang-format`, `w64-mingw32`, `gperf` |

### Building

To compile the release build on Linux:

```bash
make PLATFORM=linux MODE=release
```

> **Note:** If you modify `res/keywords.gperf`, regenerate the header file before building:  
> `gperf -N get_keyword_kind -t res/keywords.gperf > src/keywords.h`


### Running Trident

```bash
./trident <path-to-b-file>
```

> ⚠️ **Platform Support:** Linux is the primary target. Support for other platforms (e.g., Windows via MinGW) is experimental!

---

## 🤝 Contributing

Contributions make the open-source community an incredible place to learn, inspire, and create! **Any contributions you make are greatly appreciated.**

### Ways You Can Help

* **Report Issues:** Found a bug or edge-case? Open a detailed GitHub Issue.
* **Suggest Features:** Have ideas for optimizations or better CLI diagnostics? Let's discuss them!
* **Submit Pull Requests:** Look at open issues or improve docs, tests, and standard library bindings.
* **Cross-Platform Testing:** Help test and refine Windows (`w64-mingw32`) or macOS support.

### Getting Started

1. **Fork** this repository.
2. **Create** your feature branch : `git checkout -b feature/amazing-feature`.
3. **Commit** your changes : `git commit -m 'Add awesome new feature'`.
4. **Push** to the branch : `git push origin feature/amazing-feature`.
5. **Open** a Pull Request and describe your changes!

---

## 📚 References & Resources

* [Language Frontend with LLVM](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/) — Kaleidoscope
* [Users' Reference to B](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/kbman.html) — Dennis M. Ritchie
* [Parse Tree (Wikipedia)](https://en.wikipedia.org/wiki/Parse_tree) — WikiPedia
* [x86_64 Assembly Guide](https://www.cs.virginia.edu/~evans/cs216/guides/x86.html) — University of Virginia
* [Linux Syscall Reference](https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md) — chromium OS

---

## 📄 License

Distributed under the **Apache-2.0 License**. See the [`LICENSE`](https://raw.githubusercontent.com/parixitsapkota/tridentc/refs/heads/main/LICENSE) file for more information.
