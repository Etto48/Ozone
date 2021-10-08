#pragma once
#include "multitasking.h"

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
};