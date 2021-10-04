
.section .text
.code64
.global long_mode_start
long_mode_start:
    pushq $0
	popfq
    //load null into data segment registers
    mov $0, %ax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %rdi
///call global initializers
	movabs $__init_array_start, %rbx
1:	cmpq $__init_array_end, %rbx
	je 2f
	call *(%rbx)
	addq $8, %rbx
	jmp 1b
	// il resto dell'inizializzazione e' scritto in C++
2:	

    call init_tss

    popq %rdi
    call kmain
    hlt

