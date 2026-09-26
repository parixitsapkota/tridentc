global _start, exit, putchar
extern main

;; _start : [BRT](b runtime)
_start:
  call main
  call exit

exit:
  mov rdi, rax
  mov rax, 0x3C
  syscall

;; putchar : Libb
putchar:
  push rbp
  mov rbp, rsp
  push rdi
  mov rax, 1
  mov rdi, 1
  mov rsi, rsp
  mov rdx, 1
  syscall
  pop rdi
  leave
  ret
