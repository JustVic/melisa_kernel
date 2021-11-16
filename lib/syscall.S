.text
.global writeu
.global sleepu
.global exitu
.global waitu
.global keyboard_readu
.global get_total_memoryu
.global open_file
.global read_file
.global get_file_size
.global close_file
.global fork
.global exec
.global read_root_directory

writeu:
        sub $16,%rsp
        xor %eax,%eax

        mov %rdi,(%rsp)
        mov %rsi,8(%rsp)

        mov $2,%rdi
        mov %rsp,%rsi
        int $0x80

        add $16,%rsp
        ret

sleepu:
	sub $8,%rsp
	mov $1,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp
	ret

exitu:
	mov $2,%eax
	mov $0,%rdi

	int $0x80

	ret

waitu:
	sub $8,%rsp
	mov $3,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp
	ret

keyboard_readu:
	mov $4, %eax
	xor %edi,%edi

	int $0x80

	ret

get_total_memoryu:
	mov $5,%eax
	xor %edi,%edi

	int $0x80

	ret

open_file:
	sub $8,%rsp
	mov $6,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp

	ret

read_file:
	sub $24,%rsp
	mov $7,%eax

	mov %rdi,(%rsp)
	mov %rsi,8(%rsp)
	mov %rdx,16(%rsp)

	mov $3,%rdi
	mov %rsp,%rsi

	int $0x80

	add $24,%rsp

	ret

get_file_size:
	sub $8,%rsp
	mov $8,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp

	ret

close_file:
	sub $8,%rsp
	mov $9,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp

	ret

fork:
	mov $10,%eax
	xor %edi,%edi
	int $0x80

	ret

exec:
	sub $8,%rsp
	mov $11,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp
	ret

read_root_directory:
	sub $8,%rsp
	mov $12,%eax

	mov %rdi,(%rsp)
	mov $1,%rdi
	mov %rsp,%rsi

	int $0x80

	add $8,%rsp
	ret
