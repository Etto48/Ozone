#include <ozone.h>
#include <stdint.h>
#include "multitasking.h"

namespace stdin
{
    extern bool is_waiting;
    void init();
    void enable();
    void get_handle(ozone::pid_t req);
    void release_handle(ozone::pid_t req);
    ozone::pid_t check_handle();
    void get_char(ozone::pid_t pid);
    void return_char(char c);
};