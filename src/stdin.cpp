#include "include/stdin.h"

namespace stdin
{
    ozone::semid_t stdin_mutex = ozone::INVALID_SEMAPHORE;
    ozone::pid_t last_handle_request = ozone::INVALID_PROCESS;
    bool is_waiting = false;
    void init()
    {
        stdin_mutex = multitasking::create_semaphore(0);
    }
    void enable()
    {
        multitasking::release_semaphore(stdin_mutex);
    }
    void get_handle(ozone::pid_t req)
    {
        if(req!=check_handle())
        {
            debug::log(debug::level::inf,"Process %uld requested STDIN handle",req);
            multitasking::acquire_semaphore(stdin_mutex);
            last_handle_request = req;
        }
    }
    void release_handle(ozone::pid_t req)
    {
        if(req==check_handle())
        {
            debug::log(debug::level::inf,"Process %uld released STDIN handle",req);
            multitasking::release_semaphore(stdin_mutex);
        }
        last_handle_request = ozone::INVALID_PROCESS;
    }
    ozone::pid_t check_handle()
    {
        if(multitasking::semaphore_array[stdin_mutex].count==1)
        {
            return ozone::INVALID_PROCESS;
        }
        else
        {
            return last_handle_request;
        }
    }
    void get_char(ozone::pid_t pid)
    {
        if(pid==check_handle())
        {
            is_waiting = true;
            multitasking::drop();
        }
    }
    void return_char(char c)
    {
        auto pid = check_handle();
        if(pid != ozone::INVALID_PROCESS && is_waiting)
        {
            multitasking::add_ready(pid);
            is_waiting = false;
            multitasking::process_array[pid].context.rax = uint64_t(c);
        }
    }
};