section .text
global start
extern main

start:
	call main
	jmp $
