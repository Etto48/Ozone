.section .text
.code32
.macro find_cpu_descriptor
    #get lapic_id
    mov $1, %eax
    cpuid
    shrl $24, %ebx
    #search the correct descriptor
    mov cpu_array_pointer, %eax
1:  cmp 2(%eax), %ebx
    je 2f
    add sizeof_cpu_descriptor, %eax
    jmp 1b
2:  #found
.endm

.global ap_startup
ap_startup:
    #find and set stack
    find_cpu_descriptor
    add stack_base_offset, %eax
    mov %eax, %esp
    mov %eax, %ebp
    
    #copy paging from the main trie
    call enable_paging
    
    #find and set gdt
    find_cpu_descriptor
    add gdtr_offset, %eax
    #ENTER 64BIT LOADING GDT
    lgdt (%eax)
    ljmp $8, $ap_long_mode_start

.align 0x1000
.code64
ap_long_mode_start:
    pushq $0
	popfq
    #load null into data segment registers
    mov $0, %ax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    call ap64main
h:    #we should not get there but whatever
    hlt
    jmp h

.section .bss
.global ap_tmp_stack
ap_tmp_stack:
.skip 0x1000 * (32/4) * 32
