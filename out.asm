; generated from examples/hello_world.b
default rel

section .text

global puts
puts:
  push rbp
  mov rbp, rsp
  sub rsp, 80
  mov [rbp - 8], rdi
.L1:
  lea rax, [rbp - 80]
  mov [rbp - 16], rax
  mov rax, [rbp - 8]
  mov rcx, [rbp - 16]
  mov [rcx], rax
  mov rax, [rbp - 16]
  mov [rbp - 8], rax
.L2:
  mov rax, [rbp - 8]
  mov rax, [rax]
  mov [rbp - 24], rax
  mov rax, [rbp - 24]
  test rax, rax
  jnz .L3
  jmp .L4
.L3:
  mov rax, [rbp - 8]
  mov rax, [rax]
  mov [rbp - 32], rax
  mov rdi, [rbp - 32]
  xor eax, eax
  call putchar
  mov [rbp - 40], rax
  mov rax, [rbp - 8]
  mov rax, [rax]
  mov [rbp - 48], rax
  mov rax, 1
  mov [rbp - 56], rax
  mov rax, [rbp - 48]
  mov rcx, [rbp - 56]
  add rax, rcx
  mov [rbp - 64], rax
  mov rax, [rbp - 64]
  mov rcx, [rbp - 8]
  mov [rcx], rax
  jmp .L2
.L4:
  mov rax, 0
  mov [rbp - 72], rax
  mov rax, [rbp - 72]
  leave
  ret

global main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 32
.L1:
  lea rax, [rel __ro_data_1]
  mov [rbp - 8], rax
  mov rdi, [rbp - 8]
  xor eax, eax
  call puts
  mov [rbp - 16], rax
  mov rax, 0
  mov [rbp - 24], rax
  mov rax, [rbp - 24]
  leave
  ret

global _start
_start:
  call main
  mov rdi, rax
  mov rax, 0x3C
  syscall

putchar:
  push rbp
  mov rbp, rsp
  mov [rbp - 8], rdi
  push rdi
  mov rax, 1
  mov rdi, 1
  mov rsi, rsp
  mov rdx, 1
  syscall
  pop rdi
  leave
  ret

section .rodata

__ro_data_1: db 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a, 0x00

section .note.GNU-stack noalloc noexec nowrite progbits
