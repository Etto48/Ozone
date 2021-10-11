.text
.code32
.global ap_startup
ap_startup:
    call enable_paging
    #ENTER 64BIT LOADING GDT
    call get_gdt
    hlt
    
    lgdt (%eax)
    ljmp $8, $ap_long_mode_start

get_gdt:

    ret


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

    hlt

.data
