#pragma once
#include <stdint.h>
#include <ozone.h>

namespace mouse
{
    struct delta
    {
        int32_t x = 0, y = 0;
    };
    struct btns
    {
        bool middle = false, right = false, left = false;
    };
    bool wait_for_event();
    delta get_mov();
    btns get_btns();
};