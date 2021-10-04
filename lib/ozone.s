.section .text
.global fin
fin:
    mov %rax, %rdx
    mov $2, %rsi#exit
    int $0x80
    jmp fin
