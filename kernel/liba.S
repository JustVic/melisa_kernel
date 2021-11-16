.text
.global memset
.global memcpy
.global memmove
.global memcmp

memset:
    cld
    movl %edx, %ecx
    movb %sil, %al
    rep stos %al,%es:(%rdi)
    ret

memcmp:
    cld
    xorl %eax,%eax
    movl %edx,%ecx
    repe cmpsb
    setnzb %al
    ret

memcpy:
memmove:
    cld
    cmp %rdi,%rsi
    jae memmove.copy
    mov %rsi, %r8
    add %rdx, %r8
    cmp %rdi, %r8
    jbe memmove.copy

memmove.overlap:
    std
    addq %rdx,%rdi
    addq %rdx,%rsi
    subq $1,%rdi
    subq $1,%rsi

memmove.copy:
    movl %edx,%ecx
    rep movsb
    cld
    ret


