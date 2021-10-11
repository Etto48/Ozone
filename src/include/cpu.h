#pragma once
#include "multitasking.h"
#include "memory.h"
#include "clock.h"

#define MAX_CPU_COUNT 32

namespace cpu
{
    struct cpu_descriptor_t 
    {
        uint8_t processor_id;
        bool is_present = false;
        uint8_t lapic_id;
        multitasking::process_descriptor_t* process_list = nullptr;
    };
    extern cpu_descriptor_t cpu_array[MAX_CPU_COUNT];
    uint64_t get_count();
    void start_cores();
    extern "C" void ap_trampoline();
    extern "C" void ap_startup();
    extern "C" uint8_t bspdone;
    extern "C" volatile uint8_t aprunning;
    uint8_t get_current_id();

};