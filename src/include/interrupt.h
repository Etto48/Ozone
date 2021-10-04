#pragma once
#include <stdint.h>
#include "memory.h"
#include "printing.h"
#include "apic.h"

namespace interrupt
{
    constexpr uint16_t GDT_SYSTEM_CODE_SEGMENT = 0x08;
    constexpr uint16_t GDT_USER_CODE_SEGMENT = 0x10 | 3;
    constexpr uint16_t GDT_USER_DATA_SEGMENT = 0x18 | 3;
    enum class privilege_level_t
    {
        system, //0x08 GDT OFFSET
        user    //0x10 GDT OFFSET
    };
    enum class idt_gate_type_t
    {
        interrupt_gate,
        call_gate,
        trap_gate
    };
    struct context_t
    {
        uint64_t rax, rbx, rcx, rdx,
            rsi, rdi,
            rbp,
            r8, r9, r10, r11, r12, r13, r14, r15,
            fs, gs,
            int_num, int_info,
            rip, cs, rflags, rsp, ss;
    } __attribute__((packed));

    struct idt_entry_t
    {
        uint16_t offset_low : 16;
        uint16_t code_segment_selector : 16;
        uint8_t ist : 8;
        uint8_t type_and_attributes : 8;
        uint16_t offset_middle : 16;
        uint32_t offset_high : 32;
        uint32_t zero : 32;
        idt_entry_t(
            void (*offset)(),
            privilege_level_t interrupt_level = privilege_level_t::system,
            privilege_level_t caller_level = privilege_level_t::system,
            idt_gate_type_t type = idt_gate_type_t::interrupt_gate,
            bool is_present = true);
        idt_entry_t() = default;
    } __attribute__((packed));

    struct idtr_t
    {
        uint16_t limit;
        idt_entry_t *offset;
    } __attribute__((packed));

    constexpr uint64_t MAX_DRIVER_FUNCTIONS = 10;

    struct io_descriptor_t
    {
        uint64_t id;
        bool is_present;
        bool is_ready;
        void (*fin_address)();
        uint64_t (*function_array[MAX_DRIVER_FUNCTIONS])();
        io_descriptor_t()
        {
            is_present = false;
            is_ready = false;
            fin_address = nullptr;
            for (auto &f : function_array)
                f = nullptr;
        };
        io_descriptor_t(uint64_t pid, void (*fin_addr)())
        {
            id = pid;
            is_present = true;
            is_ready = false;
            fin_address = fin_addr;
            for (auto &f : function_array)
                f = nullptr;
        };
    };

    constexpr uint64_t IDT_SIZE = 256;
    constexpr uint64_t ISR_SIZE = 32;
    constexpr uint64_t IRQ_SIZE = 24;

    extern idtr_t IDTR;
    extern idt_entry_t IDT[IDT_SIZE];

    extern io_descriptor_t io_descriptor_array[IRQ_SIZE];

    extern "C" void load_idt(idtr_t &idtr);
    extern "C" void *isr_handler(context_t *context); //returns the address of the context_t of the new process
    extern "C" void *irq_handler(context_t *context);
    extern "C" void *unknown_interrupt(context_t *context);
    void init_interrupts();

    void set_driver(uint64_t irq_number, uint64_t process_id, void(*fin_addr)());
    void set_driver_function(uint64_t irq_number, uint64_t function_number, uint64_t (*function)());
    extern void (*irq_callbacks[IRQ_SIZE])(context_t *context);
    extern const char *isr_messages[];
};

#include "multitasking.h"
