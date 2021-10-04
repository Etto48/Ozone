.section .text

.macro pushaq
    push %gs
    push %fs
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rbp
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %rbx
    push %rax
.endm

.macro popaq
    pop %rax
    pop %rbx
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    pop %rbp
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    pop %fs
    pop %gs
.endm

.set RAX,       0x0
.set RBX,       0x8
.set RCX,       0x10
.set RDX,       0x18
.set RSI,       0x20
.set RDI,       0x28
.set RBP,       0x30
.set R8,        0x38
.set R9,        0x40
.set R10,       0x48
.set R11,       0x50
.set R12,       0x58
.set R13,       0x60
.set R14,       0x68
.set R15,       0x70
.set FS,        0x78
.set GS,        0x80
.set INTNUM,    0x88
.set INTINFO,   0x90
.set RIP,       0x98
.set CS,        0xa0
.set RFLAGS,    0xa8
.set RSP,       0xb0
.set SS,        0xb8

.macro proc_to_sys
    pushaq
    mov %rsp, %rdi 

    mov system_common_stack, %rsp
    #mov tss_stack_pointer, %rsp
    #mov tss_stack_pointer, %rbp
.endm

.macro sys_to_proc

    mov sys_stack_base, %r8
    mov %r8, tss_stack_pointer
    
    mov GS(%rax), %gs
    mov FS(%rax), %fs
    mov R15(%rax), %r15
    mov R14(%rax), %r14
    mov R13(%rax), %r13
    mov R12(%rax), %r12
    mov R11(%rax), %r11
    mov R10(%rax), %r10
    mov R9(%rax), %r9
    mov R8(%rax), %r8
    mov RBP(%rax), %rbp
    mov RDI(%rax), %rdi
    mov RSI(%rax), %rsi
    mov RDX(%rax), %rdx
    mov RCX(%rax), %rcx
    mov RBX(%rax), %rbx

    pushq SS(%rax)
    pushq RSP(%rax)
    pushq RFLAGS(%rax)
    pushq CS(%rax)
    pushq RIP(%rax)

    mov RAX(%rax), %rax

.endm

.global load_idt
//void load_idt(idtr_t& idtr)
load_idt:
	lidt (%rdi)
	ret

.global unknown_interrupt_wrapper
unknown_interrupt_wrapper:
    pushq $0
    pushq $-1
    proc_to_sys
    call unknown_interrupt
    sys_to_proc
    iretq

.global isr0
.global isr1
.global isr2
.global isr3
.global isr4
.global isr5
.global isr6
.global isr7
.global isr8
.global isr9
.global isr10
.global isr11
.global isr12
.global isr13
.global isr14
.global isr15
.global isr16
.global isr17
.global isr18
.global isr19
.global isr20
.global isr21
.global isr22
.global isr23
.global isr24
.global isr25
.global isr26
.global isr27
.global isr28
.global isr29
.global isr30
.global isr31
isr0:
    pushq $0
    pushq $0 
    jmp isr_common_handler_wrapper
isr1:
    pushq $0
    pushq $1 
    jmp isr_common_handler_wrapper
isr2:
    pushq $0
    pushq $2 
    jmp isr_common_handler_wrapper
isr3:
    pushq $0
    pushq $3 
    jmp isr_common_handler_wrapper
isr4:
    pushq $0
    pushq $4 
    jmp isr_common_handler_wrapper
isr5:
    pushq $0
    pushq $5 
    jmp isr_common_handler_wrapper
isr6:
    pushq $0
    pushq $6 
    jmp isr_common_handler_wrapper
isr7:
    pushq $0
    pushq $7 
    jmp isr_common_handler_wrapper
isr8:
    //pushq $0
    pushq $8 
    jmp isr_common_handler_wrapper
isr9:
    pushq $0
    pushq $9 
    jmp isr_common_handler_wrapper
isr10:
    //pushq 1
    pushq $10
    jmp isr_common_handler_wrapper
isr11:
    //pushq 1
    pushq $11
    jmp isr_common_handler_wrapper
isr12:
    //pushq 1
    pushq $12
    jmp isr_common_handler_wrapper
isr13:
    //pushq 1
    pushq $13
    jmp isr_common_handler_wrapper
isr14:
    //pushq 1
    pushq $14
    jmp isr_common_handler_wrapper
isr15:
    pushq $0
    pushq $15
    jmp isr_common_handler_wrapper
isr16:
    pushq $0
    pushq $16
    jmp isr_common_handler_wrapper
isr17:
    pushq $0
    pushq $17
    jmp isr_common_handler_wrapper
isr18:
    pushq $0
    pushq $18
    jmp isr_common_handler_wrapper
isr19:
    pushq $0
    pushq $19
    jmp isr_common_handler_wrapper
isr20:
    pushq $0
    pushq $20
    jmp isr_common_handler_wrapper
isr21:
    pushq $0
    pushq $21
    jmp isr_common_handler_wrapper
isr22:
    pushq $0
    pushq $22
    jmp isr_common_handler_wrapper
isr23:
    pushq $0
    pushq $23
    jmp isr_common_handler_wrapper
isr24:
    pushq $0
    pushq $24
    jmp isr_common_handler_wrapper
isr25:
    pushq $0
    pushq $25
    jmp isr_common_handler_wrapper
isr26:
    pushq $0
    pushq $26
    jmp isr_common_handler_wrapper
isr27:
    pushq $0
    pushq $27
    jmp isr_common_handler_wrapper
isr28:
    pushq $0
    pushq $28
    jmp isr_common_handler_wrapper
isr29:
    pushq $0
    pushq $29
    jmp isr_common_handler_wrapper
isr30:
    pushq $0
    pushq $30
    jmp isr_common_handler_wrapper
isr31:
    pushq $0
    pushq $31
    jmp isr_common_handler_wrapper

isr_common_handler_wrapper:
    proc_to_sys
    call isr_handler
    sys_to_proc
    iretq

.global irq0
.global irq1
.global irq2
.global irq3
.global irq4
.global irq5
.global irq6
.global irq7
.global irq8
.global irq9
.global irq10
.global irq11
.global irq12
.global irq13
.global irq14
.global irq15
.global irq16
.global irq17
.global irq18
.global irq19
.global irq20
.global irq21
.global irq22
.global irq23

irq0:
    pushq $0
    pushq $32
    jmp irq_common_handler_wrapper
irq1:
    pushq $0
    pushq $33
    jmp irq_common_handler_wrapper
irq2:
    pushq $0
    pushq $34
    jmp irq_common_handler_wrapper
irq3:
    pushq $0
    pushq $35
    jmp irq_common_handler_wrapper
irq4:
    pushq $0
    pushq $36
    jmp irq_common_handler_wrapper
irq5:
    pushq $0
    pushq $37
    jmp irq_common_handler_wrapper
irq6:
    pushq $0
    pushq $38
    jmp irq_common_handler_wrapper
irq7:
    pushq $0
    pushq $39
    jmp irq_common_handler_wrapper
irq8:
    pushq $0
    pushq $40
    jmp irq_common_handler_wrapper
irq9:
    pushq $0
    pushq $41
    jmp irq_common_handler_wrapper
irq10:
    pushq $0
    pushq $42
    jmp irq_common_handler_wrapper
irq11:
    pushq $0
    pushq $43
    jmp irq_common_handler_wrapper
irq12:
    pushq $0
    pushq $44
    jmp irq_common_handler_wrapper
irq13:
    pushq $0
    pushq $45
    jmp irq_common_handler_wrapper
irq14:
    pushq $0
    pushq $46
    jmp irq_common_handler_wrapper
irq15:
    pushq $0
    pushq $47
    jmp irq_common_handler_wrapper
irq16:
    pushq $0
    pushq $48
    jmp irq_common_handler_wrapper
irq17:
    pushq $0
    pushq $49
    jmp irq_common_handler_wrapper
irq18:
    pushq $0
    pushq $50
    jmp irq_common_handler_wrapper
irq19:
    pushq $0
    pushq $51
    jmp irq_common_handler_wrapper
irq20:
    pushq $0
    pushq $52
    jmp irq_common_handler_wrapper
irq21:
    pushq $0
    pushq $53
    jmp irq_common_handler_wrapper
irq22:
    pushq $0
    pushq $54
    jmp irq_common_handler_wrapper
irq23:
    pushq $0
    pushq $55
    jmp irq_common_handler_wrapper


irq_common_handler_wrapper:
    proc_to_sys
    call irq_handler
    sys_to_proc
    iretq

.global sys_call_wrapper
sys_call_wrapper:
    pushq $0
    pushq $0x80
    proc_to_sys
    call sys_call #sys_call number expected in rsi
    sys_to_proc
    iretq

.global sys_call_wrapper_system
sys_call_wrapper_system:
    pushq $0
    pushq $0x81
    proc_to_sys
    call sys_call_system #sys_call_system number expected in rsi
    sys_to_proc
    iretq

system_common_stack:
    .quad stack_top

