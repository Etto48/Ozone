#pragma once
#include <io.h>
#include "interrupt.h"
#include "video.h"
#include "math.h"
#include "debug.h"

namespace mouse
{
    void init();
    void callback(interrupt::context_t* context);
    video::v2i get_pos();
    bool left_button();
    bool right_button();
    bool middle_button();
};