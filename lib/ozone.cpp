#include "ozone.h"

namespace ozone
{
    namespace user
    {
        uint64_t sys_call_n(uint64_t sys_call_number,uint64_t arg0,uint64_t arg1,uint64_t arg2)
        {
            uint64_t ret;
            asm("mov %0, %%rdx"::"r"(arg0));
            asm("mov %0, %%rcx"::"r"(arg1));
            asm("mov %0, %%r8"::"r"(arg2));
            asm("mov %0, %%rsi"::"r"(sys_call_number));
            asm volatile("int $0x80");
            asm("mov %%rax, %0" : "=r"(ret));
            return ret;
        }
        pid_t get_id()
        {
            return sys_call_n(0);
        }
        void sleep(uint64_t ticks)
        {
            sys_call_n(1,ticks);
        }
        void exit(uint64_t ret)
        {
            sys_call_n(2,ret);
        }
        semid_t create_semaphore(int64_t start_count)
        {
            return sys_call_n(3,(uint64_t)start_count);
        }
        void acquire_semaphore(semid_t semaphore_id)
        {
            sys_call_n(4,semaphore_id);
        }
        void release_semaphore(semid_t semaphore_id)
        {
            sys_call_n(5,semaphore_id);
        }
        uint64_t join(pid_t id)
        {
            return sys_call_n(9,id);
        }
        pid_t driver_call(uint64_t driver_number, uint64_t function_number)
        {
            return sys_call_n(10,driver_number,function_number);
        }
        bool signal(signal_t signal,pid_t process_id)
        {
            return sys_call_n(11,(uint64_t)signal,process_id);
        }
        void set_signal_handler(signal_t signal, void(*handler)())
        {
            sys_call_n(12,(uint64_t)signal,(uint64_t)handler,(uint64_t)return_from_signal);
        }
        void return_from_signal()
        {
            sys_call_n(13);
        }
        shmid_t shm_get(bool user_rw, size_t size)
        {
            return sys_call_n(14,user_rw,size);
        }
        void shm_wait_and_destroy(shmid_t shmem_id)
        {
            sys_call_n(15,shmem_id);
        }
        void* shm_attach(shmid_t shmem_id)
        {
            return (void*)sys_call_n(16,shmem_id);
        }
        bool shm_detach(shmid_t shmem_id)
        {
            return sys_call_n(17,shmem_id);
        }
        terminal_size_t get_stdout_handle()
        {
            auto ret = sys_call_n(18);
            return {uint16_t((ret & 0xFFFF0000)>>16),uint16_t(ret)};
        }
        void release_stdout_handle()
        {
            sys_call_n(19);
        }
        void put_char(char c,uint64_t color, uint16_t x, uint16_t y)
        {
            sys_call_n(
                20,
                uint64_t(c) |
                uint64_t(x)<<32 |
                uint64_t(y)<<(32+16),
                color
                );
        }
        void get_stdin_handle()
        {
            sys_call_n(21);
        }
        void release_stdin_handle()
        {
            sys_call_n(22);
        }
        char get_char()
        {
            return char(sys_call_n(23));
        }
    };

    namespace system
    {
        uint64_t sys_call_n(uint64_t sys_call_number,uint64_t arg0,uint64_t arg1,uint64_t arg2,uint64_t arg3)
        {
            uint64_t ret;
            asm("mov %0, %%rdx"::"r"(arg0));
            asm("mov %0, %%rcx"::"r"(arg1));
            asm("mov %0, %%r8"::"r"(arg2));
            asm("mov %0, %%r9"::"r"(arg3));
            asm("mov %0, %%rsi"::"r"(sys_call_number));
            asm volatile("int $0x81");
            asm("mov %%rax, %0" : "=r"(ret));
            return ret;
        }

        void set_driver(uint64_t irq_number,pid_t process_id)
        {
            sys_call_n(0,irq_number,process_id,(uint64_t)user::fin);
        }
        uint64_t wait_for_interrupt()
        {
            return sys_call_n(1);
        }
        void set_driver_function(uint64_t irq_number,uint64_t function_id, uint64_t(*function)())
        {
            sys_call_n(2,irq_number,function_id,(uint64_t)function);
        }
    };
};

void* operator new(size_t size)
{
    return (void*)ozone::user::sys_call_n(7,(uint64_t)size);
}
void *operator new[](size_t size)
{
    return (void*)ozone::user::sys_call_n(7,(uint64_t)size);
}
//void* operator new(size_t size, size_t align)
//{
//}
void operator delete(void* address)
{
    ozone::user::sys_call_n(8,(uint64_t)address);
}
void operator delete[](void* address)
{
    ozone::user::sys_call_n(8,(uint64_t)address);   
}
void operator delete(void* address, unsigned long)
{
    ozone::user::sys_call_n(8,(uint64_t)address);
}

