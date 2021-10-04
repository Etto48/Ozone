#include <ps2_mouse.h>

namespace mouse
{
    bool wait_for_event()
    {
        auto p = ozone::INVALID_PROCESS;
        while (p == ozone::INVALID_PROCESS)
            p = ozone::user::driver_call(12, 2);
        return (bool)ozone::user::join(p);
    }
    delta get_mov()
    {
        auto p = ozone::INVALID_PROCESS;
        while (p == ozone::INVALID_PROCESS)
            p = ozone::user::driver_call(12, 0);
        auto ret = ozone::user::join(p);
        return {int32_t(ret), int32_t(ret>>32)};
    }
    btns get_btns()
    {
        auto p = ozone::INVALID_PROCESS;
        while (p == ozone::INVALID_PROCESS)
            p = ozone::user::driver_call(12, 1);
        auto ret = ozone::user::join(p);
        return {bool(ret & 1), bool(ret & (1<<1)),bool(ret & (1<<2))};
    }
};