#pragma once
#include "interrupt.h"
#include "multitasking.h"
#include "clock.h"
#include "stdout.h"
#include "stdin.h"

namespace syscalls
{
    extern "C" void* sys_call(interrupt::context_t* context, uint64_t sys_call_number);//handler
    extern "C" void* sys_call_system(interrupt::context_t* context, uint64_t sys_call_number);//handler (only system level)
};