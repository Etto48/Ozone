.section .text
.global set_current_trie
set_current_trie:
    mov %cr3, %r15
    cmp %r15, %rdi
    je .end
    mov %rdi, %cr3
.end: 
    ret

.global get_current_trie
get_current_trie:
    mov %cr3, %rax
    ret

.global get_cr2
get_cr2:
    mov %cr2, %rax
    ret
