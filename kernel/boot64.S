#include "kernel.h"
#include "sizes.h"
#include "multiboot2.h"


#define KERNEL_VMA   0xffff800000000000

.section .data

gdt64:
	.quad 0x0000000000000000 // 0x00 NULL
	.quad 0x0020980000000000 // 0x08 KCODE64
	.quad 0x0020f80000000000
	.quad 0x0000f20000000000
TssDesc2:
    .word TssLen2-1
    .word 0
    .byte 0
    .byte 0x89
    .byte 0
    .byte 0
    .quad 0

gdt64_end:

.global init_gdt64_ptr_baseaddr2
.global init_gdt64_ptr2
init_gdt64_ptr2:
        .word gdt64_end - gdt64 - 1
init_gdt64_ptr_baseaddr2:
        .quad gdt64


.global Tss2
.global TssDesc2
Tss2:
	.long 0
	.quad 0xffff800000190000
	.fill 88
	.long TssLen2

.equ TssLen2, . - Tss2



.extern init_gdt64_ptr_baseaddr
.extern init_gdt64_ptr
.extern Tss
.extern TssDesc


.code64

.section .text

SetTss:
    lea Tss2(%rip), %rax
    lea TssDesc2(%rip), %rbx
    mov %ax, 2(%rbx)
    shr $16,%rax
    mov %al,4(%rbx)
    shr $8,%rax
    mov %al,7(%rbx)
    shr $8,%rax
    mov %eax,8(%rbx)
    mov $0x20,%ax
    ltr %ax
    ret

.global _start64h
_start64h:

    lea init_gdt64_ptr2(%rip), %rbx
    lgdt (%rbx)         # Reload the GDT relative to its high memory address

    movq $KERNEL_VMA, %rax
    add %rax, %rsp

    add %rax, %rsp      # You were already adjusting the stack, I just moved it

    call SetTss
    // Setup segment selectors
    movw $0, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movq $KERNEL_VMA, %rax
    addq %rax, %rdi

//    movq   $0x0, p4_table
//    invlpg 0

    //call InitPIT

    call InitPIC

    call cmain

1:
	sti
	hlt
    jmp 1b


InitPIT:
    mov $(1<<2)|(3<<4), %al
    out %al, $0x43

    mov $11931,%ax
    out %al, $0x40
    mov %ah,%al
    out %al, $0x40
	ret

InitPIC:
    mov $0x11,%al
    out %al, $0x20
    out %al, $0xa0

    mov $32,%al
    out %al, $0x21
    mov $40,%al
    out %al, $0xa1

    mov $4,%al
    out %al, $0x21
    mov $2,%al
    out %al, $0xa1

    mov $1,%al
    out %al, $0x21
    out %al, $0xa1

    mov $0b11111100,%al
    out %al, $0x21
    mov $0b11111111,%al
    out %al, $0xa1

    ret
