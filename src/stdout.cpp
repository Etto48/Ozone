#include "include/stdout.h"

namespace stdout
{
    ozone::semid_t stdout_mutex = ozone::INVALID_SEMAPHORE;
    ozone::pid_t last_handle_request = ozone::INVALID_PROCESS;
    terminal_size_t last_terminal_size = {80,25};
    void init()
    {
        stdout_mutex = multitasking::create_semaphore(0);
    }
    void enable()
    {
        multitasking::release_semaphore(stdout_mutex);
    }
    terminal_size_t get_handle(ozone::pid_t req)
    {
        if(req!=check_handle())
        {
            multitasking::acquire_semaphore(stdout_mutex);
            last_handle_request = req;
            debug::log(debug::level::inf,"Process %uld requested STDOUT handle",req);
        }
        auto ss = video::get_screen_size();
        last_terminal_size = {uint16_t(ss.x/8),uint16_t(ss.y/8)};   
        return last_terminal_size;
    }
    void release_handle(ozone::pid_t req)
    {
        if(req==check_handle())
        {
            multitasking::release_semaphore(stdout_mutex);
            debug::log(debug::level::inf,"Process %uld released STDOUT handle",req);
        }
        last_handle_request = ozone::INVALID_PROCESS;
    }
    ozone::pid_t check_handle()
    {
        if(multitasking::semaphore_array[stdout_mutex].count==1)
        {
            return ozone::INVALID_PROCESS;
        }
        else
        {
            return last_handle_request;
        }
    }
    void put_char(ozone::pid_t pid, char c, uint64_t color, uint16_t x, uint16_t y)
    {
        if(pid==check_handle())
        {
            if(x<last_terminal_size.x&&y<last_terminal_size.y)
            {
                draw_char(c,color,x,y);
            }
        }
    }   
};