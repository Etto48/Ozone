.code64
.section .text

.global _start
_start:
    push %rdi
    push %rsi
    //call global initializers

	lea __preinit_array_start(%rip), %rbx
1:	lea __preinit_array_end(%rip), %rcx
    cmpq %rcx, %rbx
	je 2f
	call *(%rbx)
	addq $8, %rbx
	jmp 1b
2:

	lea __init_array_start(%rip), %rbx
1:	lea __init_array_end(%rip), %rcx
    cmpq %rcx, %rbx
	je 2f
	call *(%rbx)
	addq $8, %rbx
	jmp 1b
2:

    pop %rsi
    pop %rdi
    lea wrapper(%rip), %rax
    call *%rax

.global fini
fini:

    push %rdi
    push %rsi

    	lea __fini_array_start(%rip), %rbx
    1:	lea __fini_array_end(%rip), %rcx
        cmpq %rcx, %rbx
	    je 2f
    	call *(%rbx)
    	addq $8, %rbx
    	jmp 1b
    2:
    
    pop %rsi
    pop %rdi
    ret

