#include "include/interrupt.h"

namespace interrupt
{

    extern "C"
    {
        void isr0();
        void isr1();
        void isr2();
        void isr3();
        void isr4();
        void isr5();
        void isr6();
        void isr7();
        void isr8();
        void isr9();
        void isr10();
        void isr11();
        void isr12();
        void isr13();
        void isr14();
        void isr15();
        void isr16();
        void isr17();
        void isr18();
        void isr19();
        void isr20();
        void isr21();
        void isr22();
        void isr23();
        void isr24();
        void isr25();
        void isr26();
        void isr27();
        void isr28();
        void isr29();
        void isr30();
        void isr31();

        void irq0();
        void irq1();
        void irq2();
        void irq3();
        void irq4();
        void irq5();
        void irq6();
        void irq7();
        void irq8();
        void irq9();
        void irq10();
        void irq11();
        void irq12();
        void irq13();
        void irq14();
        void irq15();
        void irq16();
        void irq17();
        void irq18();
        void irq19();
        void irq20();
        void irq21();
        void irq22();
        void irq23();

        void unknown_interrupt_wrapper();
        void sys_call_wrapper();
        void sys_call_wrapper_system();
    }

    void (*irq_callbacks[IRQ_SIZE])(context_t *context);

    io_descriptor_t io_descriptor_array[IRQ_SIZE];

    constexpr uint16_t DPL_SYSTEM = 0;
    constexpr uint64_t DPL_USER = 0b01100000;

    idt_entry_t::idt_entry_t(void (*offset)(), privilege_level_t interrupt_level, privilege_level_t caller_level, idt_gate_type_t type, bool is_present)
    {
        uint64_t _offset = (uint64_t)offset;
        offset_low = (uint16_t)(_offset & 0xffff);
        offset_middle = (uint16_t)((_offset & 0xffff0000) >> 16);
        offset_high = (uint32_t)((_offset & 0xffffffff00000000) >> 32);
        switch (interrupt_level)
        {
        case privilege_level_t::system:
            code_segment_selector = GDT_SYSTEM_CODE_SEGMENT;
            break;
        case privilege_level_t::user:
            code_segment_selector = GDT_USER_CODE_SEGMENT;
            break;
        }
        type_and_attributes = 0;
        switch (caller_level)
        {
        case privilege_level_t::system:
            type_and_attributes |= DPL_SYSTEM;
            break;
        case privilege_level_t::user:
            type_and_attributes |= DPL_USER;
            break;
        }

        if (is_present)
        {
            switch (type)
            {
            case idt_gate_type_t::interrupt_gate:
                type_and_attributes |= 0b10001110;
                break;
            case idt_gate_type_t::call_gate:
                type_and_attributes |= 0b10001100;
                break;
            case idt_gate_type_t::trap_gate:
                type_and_attributes |= 0b10001111;
                break;
            }
        }

        ist = 0;
        zero = 0;
    }
    idtr_t IDTR;
    idt_entry_t IDT[IDT_SIZE];

    void init_interrupts()
    {
        void (*isrs[])() =
            {
                isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
                isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
                isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
                isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31};
        void (*irqs[])() =
            {
                irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
                irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15,
                irq16, irq17, irq18, irq19, irq20, irq21, irq22, irq23};

        IDTR = {IDT_SIZE * sizeof(idt_entry_t) - 1, IDT};
        apic::disable_8259();
        for (auto &c : irq_callbacks)
        {
            c = nullptr;
        }
        memory::memset(IDT, 0, IDTR.limit + 1);
        for (uint64_t i = 0; i < IDT_SIZE; i++)
        {
            IDT[i] = {unknown_interrupt_wrapper, privilege_level_t::system, privilege_level_t::system, idt_gate_type_t::interrupt_gate, true};
        }
        for (uint64_t i = 0; i < ISR_SIZE; i++)
        {
            IDT[i] = {isrs[i], privilege_level_t::system, privilege_level_t::system, idt_gate_type_t::interrupt_gate, true};
        }
        for (uint64_t i = 0; i < IRQ_SIZE; i++)
        {
            IDT[i + ISR_SIZE] = {irqs[i], privilege_level_t::system, privilege_level_t::system, idt_gate_type_t::interrupt_gate, true};
        }
        //user level syscalls
        IDT[0x80] = {sys_call_wrapper, privilege_level_t::system, privilege_level_t::user, idt_gate_type_t::interrupt_gate, true};
        //system level syscalls
        IDT[0x81] = {sys_call_wrapper_system, privilege_level_t::system, privilege_level_t::system, idt_gate_type_t::interrupt_gate, true};
        load_idt(IDTR);
        asm volatile("sti");
    }
    extern "C" void *isr_handler(context_t *context)
    {
        multitasking::save_state(context);
        bool solved = false;
        if(context->int_num == 14 && !(context->int_info & 1))//page fault, unmapped memory
        {
            //debug::log(debug::level::inf,"Process %uld requested an unmapped address",multitasking::execution_index);
            auto requested_address = paging::get_cr2();
            auto& ei = multitasking::current_execution_index();
            if(multitasking::process_array[ei].level == privilege_level_t::user)
            {
                if((uint64_t)requested_address>multitasking::stack_top_address && (uint64_t)requested_address<multitasking::stack_bottom_address)
                {//the process requested an address in its stack
                    solved = multitasking::add_pages(ei,(void*)((uint64_t)requested_address&~0xfff),paging::flags::RW | paging::flags::USER,1);
                }
                else
                {
                    debug::log(debug::level::wrn,"Process %uld requested memory over the maximum amount possible",ei);
                }
            }
        }
        else
        {
            debug::log(debug::level::wrn,"ISR not handled");
        }

        if(!solved)
            multitasking::abort(nullptr,context);
        return multitasking::load_state();
    }
    extern "C" void *irq_handler(context_t *context)
    {
        multitasking::save_state(context);
        auto irq_num = context->int_num - ISR_SIZE;
        if (irq_callbacks[irq_num])
        {
            irq_callbacks[irq_num](context);
        }
        if (io_descriptor_array[irq_num].is_present && io_descriptor_array[irq_num].is_ready)
        {
            multitasking::process_array[io_descriptor_array[irq_num].id].context.rax = multitasking::current_execution_index();
            multitasking::add_ready(io_descriptor_array[irq_num].id);
        } //only send eoi when the process calls wait_for_interrupt
        else
        {
            apic::send_EOI(context->int_num);
        }
        return multitasking::load_state();
    }
    extern "C" void *unknown_interrupt(context_t *context)
    {
        multitasking::save_state(context);
        debug::log(debug::level::wrn,"Unknown interrupt called (%uld)",context->int_num);
        return multitasking::load_state();
    }

    const char *isr_messages[] =
        {
            "Division By Zero",
            "Debug",
            "Non Maskable Interrupt",
            "Breakpoint",
            "Into Detected Overflow",
            "Out of Bounds",
            "Invalid Opcode",
            "No Coprocessor",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Bad TSS",
            "Segment Not Present",
            "Stack Fault",
            "General Protection Fault",
            "Page Fault",
            "Reserved",
            "x87 Floating Point Exception",
            "Alignment Check",
            "Machine Check",
            "SIMD Floating-Point Exception",
            "Virtualization Exception",
            "Control Protection Exception",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved",
            "Reserved"};

    void reset_driver(uint64_t irq_number)
    {
        if (irq_number < IRQ_SIZE)
        {
            io_descriptor_array[irq_number].is_present = false;
            debug::log(debug::level::inf, "Driver %ud cleared", irq_number);
        }
    }
    void set_driver(uint64_t irq_number, uint64_t process_id, void(*fin_addr)())
    {
        if (
            irq_number < IRQ_SIZE 
            && process_id < multitasking::MAX_PROCESS_NUMBER 
            && multitasking::process_array[process_id].is_present 
            && multitasking::process_array[process_id].level == interrupt::privilege_level_t::system)
        {
            io_descriptor_array[irq_number] = io_descriptor_t{process_id,fin_addr};
            debug::log(debug::level::inf, "Process %uld is set as driver %ud", process_id, irq_number);
        }
    }
    void set_driver_function(uint64_t irq_number, uint64_t function_number, uint64_t(*function)())
    {
        if(
            irq_number<IRQ_SIZE
            && io_descriptor_array[irq_number].is_present
            && function_number<MAX_DRIVER_FUNCTIONS
        )
        {
            io_descriptor_array[irq_number].function_array[function_number] = function;
        }
    }
};