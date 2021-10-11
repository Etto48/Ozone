#pragma once
#include "multitasking.h"
#include "memory.h"
#include "clock.h"
#include "heap.h"

#define MAX_CPU_COUNT 32
#define AP_KERNEL_STACK_SIZE (1024 * 1024 * 16)

namespace cpu
{
    struct gdt_entry_t
    {
        uint16_t limit = 0;
        uint16_t base_low = 0;
        uint8_t base_middle = 0;
        uint8_t access;
        uint8_t flags;
        uint8_t base_high = 0;
        gdt_entry_t(uint8_t access,uint8_t flags):access(access),flags(flags){}
    } __attribute__((packed));
    struct tss_t
    {
        uint16_t reserved0;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;
        uint64_t reserved1;
        uint64_t ist[7];
        uint64_t reserved2;
        uint8_t reserved3;
        uint8_t iopb_offset;
    }__attribute__((packed));
    struct tss_descriptor_t
    {
        uint16_t limit = sizeof(tss_t);
        uint16_t base_low;
        uint8_t base_middle_low;
        uint8_t access = 0b10001001;
        uint8_t flags = 0;
        uint8_t base_middle_high;
        uint32_t base_high;
        uint32_t zero = 0;
        tss_descriptor_t(tss_t* tss_pointer)
        {
            auto tss_addr = reinterpret_cast<uint64_t>(tss_pointer);
            base_low = uint16_t(tss_addr & 0xFFFF);
            base_middle_low = uint8_t(tss_addr>>16 & 0xFF);
            base_middle_high = uint8_t(tss_addr>>24 & 0xFF);
            base_high = uint32_t(tss_addr>>32 & 0xFFFFFFFF);
        };
    }__attribute__((packed));
    struct gdt_t
    {
        gdt_entry_t null_segment = gdt_entry_t(0,0);
        gdt_entry_t code_sys_segment = gdt_entry_t(0b10011010,0b00100000);
        gdt_entry_t code_usr_segment = gdt_entry_t(0b11111010,0b00100000);
        gdt_entry_t data_usr_segment = gdt_entry_t(0b11110010,0);
        tss_descriptor_t tss_segment = tss_descriptor_t(nullptr);
        gdt_t(tss_t* tss_pointer)
        {
            tss_segment = tss_descriptor_t(tss_pointer);
        }
    }__attribute__((packed));
    struct gdtr_t
    {
        uint16_t limit = sizeof(gdt_t)-1;
        uint64_t base;
        gdtr_t(gdt_t* gdt_pointer):base((uint64_t)gdt_pointer){}
    }__attribute__((packed));
    struct kernel_stack_t
    {
        uint8_t d[AP_KERNEL_STACK_SIZE];
    };
    struct cpu_descriptor_t 
    {
        uint8_t processor_id;
        bool is_present = false; 
        uint8_t lapic_id; 
        tss_t tss; 
        gdt_t gdt = gdt_t(&tss);
        gdtr_t gdtr = gdtr_t(&gdt);
        bool is_booted = false;
        kernel_stack_t kernel_stack;
        multitasking::process_descriptor_t* process_list = nullptr;
    };
    extern cpu_descriptor_t cpu_array[MAX_CPU_COUNT];
    uint64_t get_count();
    uint64_t get_booted_count();
    void start_cores();
    extern "C" void ap_trampoline();
    extern "C" void ap_startup();
    extern "C" uint8_t bspdone;
    extern "C" volatile uint8_t aprunning;
    extern "C" uint32_t sizeof_cpu_descriptor;
    extern "C" uint32_t cpu_array_pointer;
    extern "C" uint32_t gdtr_offset;
    extern "C" uint32_t stack_base_offset;
    extern "C" void ap64main();
    uint8_t get_current_processor_id();

};