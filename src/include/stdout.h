#pragma once

#include "video.h"
#include "multitasking.h"
#include <ozone.h>
#include "debug.h"

namespace stdout
{
    struct terminal_size_t
    {
        uint16_t x,y;
    };
    void init();
    void enable();
    terminal_size_t get_handle(ozone::pid_t req);
    void release_handle(ozone::pid_t req);
    ozone::pid_t check_handle();
    void put_char(ozone::pid_t pid, char c, uint64_t color, uint16_t x, uint16_t y);
};