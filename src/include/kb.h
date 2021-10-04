#pragma once
#include "interrupt.h"
#include "multitasking.h"
#include "stdin.h"
#include "io.h"
#include "debug.h"
#include <ozone.h>

namespace kb
{
    void init();
    void callback(interrupt::context_t *context);
};
