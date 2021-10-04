#include "include/syscalls.h"

namespace multitasking
{
    const char *syscalls_names[] =
        {
            "get_id",
            "sleep",
            "exit",
            "create_semaphore",
            "acquire_semaphore",
            "release_semaphore",
            "fork",
            "new",
            "delete",
            "join",
            "driver_call",
            "signal",
            "set_signal_handler",
            "return_from_signal",
            "shm_get",
            "shm_wait_and_destroy",
            "shm_attach",
            "shm_detach",
            "get_stdout_handle",
            "release_stdout_handle",
            "put_char",
            "get_stdin_handle",
            "release_stdin_handle",
            "get_char"
    };
};

namespace syscalls
{
    extern "C" void *sys_call(interrupt::context_t *context, uint64_t sys_call_number)
    {
        //debug::log(debug::level::inf, "SYSCALL %s",multitasking::syscalls_names[sys_call_number]);
        //rdx = arg0
        //rcx = arg1
        multitasking::save_state(context);
        context = &multitasking::process_array[multitasking::execution_index].context;
        switch (sys_call_number)
        {
        case 0: //get_id
            context->rax = multitasking::execution_index;
            break;
        case 1: //sleep
            clock::add_timer(context->rdx);
            break;
        case 2: //exit
            multitasking::exit(context->rdx);
            break;
        case 3: //create_semaphore
            context->rax = multitasking::create_semaphore(context->rdx);
            break;
        case 4: //acquire_semaphore
            multitasking::acquire_semaphore(context->rdx);
            break;
        case 5: //release_semaphore
            multitasking::release_semaphore(context->rdx);
            break;
        case 6: //fork
            context->rax = multitasking::fork(multitasking::execution_index,(void (*)())context->rdx, (void (*)())context->rcx);
            break;
        case 7: //new
            context->rax = (uint64_t)multitasking::process_array[multitasking::execution_index].process_heap.malloc(context->rdx);
            break;
        case 8: //delete
            multitasking::process_array[multitasking::execution_index].process_heap.free((void *)context->rdx);
            break;
        case 9: //join
            context->rax = multitasking::join(context->rdx);
            break;
        case 10://driver_call
            context->rax = multitasking::driver_call(context->rdx,context->rcx);
            break;
        case 11://signal
            context->rax = multitasking::signal(multitasking::execution_index,(ozone::signal_t)context->rdx,context->rcx);
            break;
        case 12://set_signal_handler
            multitasking::set_signal_handler(multitasking::execution_index,(ozone::signal_t)context->rdx,(void(*)())context->rcx,(void(*)())context->r8);
            break;
        case 13://return_from_signal
            multitasking::return_from_signal(multitasking::execution_index);
            break;
        case 14://shm_get
            context->rax = multitasking::shm_get(multitasking::execution_index,(uint16_t)context->rdx,context->rcx);
            break;
        case 15://shm_wait_and_destroy
            multitasking::shm_wait_and_destroy(context->rdx);
            break;
        case 16://shm_attach
            context->rax = (uint64_t)multitasking::shm_attach(multitasking::execution_index,context->rdx);
            break;
        case 17://shm_detach
            context->rax = multitasking::shm_detach(multitasking::execution_index,context->rdx);
            break;
        case 18://get_stdout_handle
            { 
                auto ret = stdout::get_handle(multitasking::execution_index);
                context->rax = ret.x<<16 | ret.y;
            }
            break;
        case 19://release_stdout_handle
            stdout::release_handle(multitasking::execution_index);
            break;
        case 20://put_char
            stdout::put_char(
                multitasking::execution_index,
                (context->rdx & 0x00000000000000FF),
                context->rcx,
                (context->rdx & 0x0000FFFF00000000)>>32,
                (context->rdx & 0xFFFF000000000000)>>(32+16));
        case 21://get_stdin_handle
            stdin::get_handle(multitasking::execution_index);
            break;
        case 22://release_stdout_handle
            stdin::release_handle(multitasking::execution_index);
            break;
        case 23://get_char
            stdin::get_char(multitasking::execution_index);
            break;
        default:
            break;
        }
        return multitasking::load_state();
    }

    extern "C" void *sys_call_system(interrupt::context_t *context, uint64_t sys_call_number)
    {
        //rdx = arg0
        //rcx = arg1
        //r8  = arg2
        //r9  = arg3
        multitasking::save_state(context);
        context = &multitasking::process_array[multitasking::execution_index].context;
        switch (sys_call_number)
        {
        case 0: //set_driver
            interrupt::set_driver(context->rdx,context->rcx,(void(*)())context->r8);
            break;
        case 1: //wait_for_interrupt
            for (uint64_t irq = 0; irq < interrupt::IRQ_SIZE; irq++)
            {
                auto &d = interrupt::io_descriptor_array[irq];
                if (d.id == multitasking::execution_index)
                {
                    if(d.is_present)
                    {
                        apic::send_EOI(interrupt::ISR_SIZE + irq);
                        d.is_ready = true;
                    }
                }
            }
            multitasking::drop();
            break;
        case 2:
            interrupt::set_driver_function(context->rdx, context->rcx, (uint64_t(*)())context->r8);
            break;
        default:
            break;
        }
        return multitasking::load_state();
    }
};