#include "kernel.h"
#include "sizes.h"
#include "multiboot2.h"

#define KERNEL_VMA   0xffff800000000000

.SET HEADER_LENGTH, header_end - header_start
.SET CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + HEADER_LENGTH)
.section .multiboot
.code32
.align 8
header_start:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT_ARCHITECTURE_I386
    .long HEADER_LENGTH
    .long CHECKSUM

    // multiboot tags 

    .short MULTIBOOT_HEADER_TAG_END
    .short 0    // flags, none set
    .long 8     // size, (short + short + long)
header_end:

// Global Descriptor Table
.section .data

.align 16
gdt64:
	.quad 0x0000000000000000
	.quad 0x0020980000000000 // 0x08 KCODE64
	.quad 0x0020f80000000000
	.quad 0x0000f20000000000
TssDesc:
    .word TssLen-1
    .word 0
    .byte 0
    .byte 0x89
    .byte 0
    .byte 0
    .quad 0

gdt64_end:

.align 16

.global init_gdt64_ptr_baseaddr
.global init_gdt64_ptr
init_gdt64_ptr:
        .word gdt64_end - gdt64 - 1
init_gdt64_ptr_baseaddr:
        .quad gdt64


.global Tss
.global TssDesc
Tss:
	.long 0
	.quad 0xffff800000190000
	.fill 88
	.long TssLen

.equ TssLen, . - Tss


//.data
//.global multiboot_magic,multiboot_info
multiboot_magic:
	.long 0
multiboot_info:
	.long 0

.code32

.section .text
.global _start, p4_table, p3_table
.type _start, @function
_start:
    cli

    movl $(stack_top), %esp

    mov %eax, multiboot_magic
    mov %ebx,  multiboot_info

    /* reset EFLAGS. */
    pushl $0
    popf

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call enable_paging

    call set_up_SSE

    // load the 64-bit GDT
    lgdt (init_gdt64_ptr)

    ljmp  $0x08, $(_start64)

.code64
.extern _start64h
_start64:

	mov    multiboot_magic, %rsi
	mov    multiboot_info, %rdi
	mov    $p4_table, %rdx

	/* do an indirect to force non-relative jmp */
	movabsq $_start64h, %rax
	jmp *%rax

error64:
    movl $0x4f524f45, (0xb8000)
    movl $0x4f3a4f52, (0xb8004)
    movl $0x4f204f20, (0xb8008)
    movb %al, (0xb800a)
    hlt

.code32
enable_paging:
    // load P4 to cr3 register (cpu uses this to access the P4 table)
    movl $(p4_table), %eax
    movl %eax, %cr3

    // enable PAE-flag in cr4 (Physical Address Extension)
    movl %cr4, %eax
    orl $1 << 5, %eax
    mov %eax, %cr4

    // set the long mode bit in the EFER MSR (model specific register)
    mov $0xC0000080, %ecx
    rdmsr
    orl $1 << 8, %eax
    wrmsr

    // enable paging in the cr0 register
    movl %cr0, %eax
    orl $1 << 31, %eax
    mov %eax, %cr0

    ret

// Prints `ERR: ` and the given error code to screen and hangs.
// parameter: error code (in ascii) in al
error:
    movl $0x4f524f45, (0xb8000)
    movl $0x4f3a4f52, (0xb8004)
    movl $0x4f204f20, (0xb8008)
    movb %al, (0xb800a)
    hlt

// Throw error 0 if eax doesn't contain the Multiboot 2 magic value (0x36d76289).
check_multiboot:
    cmpl $0x36d76289, %eax
    jne no_multiboot
    ret
no_multiboot:
    movb $'0', %al
    jmp error

// Throw error 1 if the CPU doesn't support the CPUID command.
check_cpuid:
    pushf                // Store the FLAGS-register.
    pop %eax             // Restore the A-register.
    mov %eax, %ecx       // Set the C-register to the A-register.
    xor $1 << 21, %eax   // Flip the ID-bit, which is bit 21.
    push %eax            // Store the A-register.
    popf                 // Restore the FLAGS-register.
    pushf                // Store the FLAGS-register.
    pop %eax             // Restore the A-register.
    push %ecx            // Store the C-register.
    popf                 // Restore the FLAGS-register.
    xor %ecx, %eax       // Do a XOR-operation on the A-register and the C-register.
    jz no_cpuid          // The zero flag is set, no CPUID.
    ret                  // CPUID is available for use.
no_cpuid:
    mov $'1', %al
    jmp error

// Throw error 2 if the CPU doesn't support Long Mode.
check_long_mode:
    movl $0x80000000, %eax  // Set the A-register to 0x80000000.
    cpuid                   // CPU identification.
    cmp $0x80000001, %eax   // Compare the A-register with 0x80000001.
    jb no_long_mode         // It is less, there is no long mode.
    movl $0x80000001, %eax  // Set the A-register to 0x80000001.
    cpuid                   // CPU identification.
    test $1 << 29, %edx     // Test if the LM-bit, which is bit 29, is set in the D-register.
    jz no_long_mode         // They aren't, there is no long mode.
    ret
no_long_mode:
    mov $'2', %al
    jmp error

// Check for SSE and enable it. If it's not supported throw a error.
set_up_SSE:
    // check for SSE
    movl $0x1, %eax
    cpuid
    testl $1<<25, %edx
    jz no_SSE

    // enable SSE
    movl %cr0, %eax
    andw  $0xFFFB, %ax      // clear coprocessor emulation CR0.EM
    orw $0x2, %ax          // set coprocessor monitoring  CR0.MP
    movl %eax, %cr0
    movl %cr4, %eax
    orw $3 << 9, %ax       // set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
    movl %eax, %cr4

    ret
no_SSE:
    movb $'a', %al
    jmp error


.section .data
  .align 4096
p4_table:
    .quad p3_table + 3
    .fill 255,8,0
    .quad p3_table + 3
    .fill 255,8,0

p3_table:
    	.quad 0x0000000000000083 // 0GB - 1GB
    	.quad 0x0000000040000083 // 1GB - 2GB


.section .bss
stack_bottom:
    .skip 4096 * 4
stack_top:

