#pragma once
#include <stdint.h>
#include <stddef.h>
#include "printing.h"
#include "multitasking.h"
#include <io.h>

namespace debug
{
    enum class level
    {
        inf,
        wrn,
        err
    };
    void log(level lev, const char* fmt, ...);
    void init();
};