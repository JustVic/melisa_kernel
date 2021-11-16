section .text
global writeu

writeu:
	sub rsp, 16
	xor eax,eax

	mov [rdi],rdi
	mov [rsp+8],rsi

	mov rdi,2
	mov rsi,rsp
	int 0x80

	add rsp,16
	ret
