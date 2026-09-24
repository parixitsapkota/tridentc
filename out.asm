; MODULE : test/test.b
default rel

section .text

global main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 8 ;; var : a
  sub rsp, 8 ;; var : b
  sub rsp, 8
  mov qword [rsp], 0
  mov rax, qword [rsp]
  mov qword [rbp - 16], rax
  add rsp, 8
  mov rax, qword [rbp - 16]
  sub rsp, 8
  mov qword [rsp], rax
  mov rax, qword [rsp]
  add rsp, 8
