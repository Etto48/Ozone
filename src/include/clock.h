#pragma once
#include <stdint.h>
#include "interrupt.h"
#include "multitasking.h"
#include "heap.h"
#include <io.h>

namespace clock
{
    void set_freq(uint64_t hz);
    void init();
    void callback(interrupt::context_t* context);

    uint64_t clean_timer_list();
    void add_timer(uint64_t ticks);
};