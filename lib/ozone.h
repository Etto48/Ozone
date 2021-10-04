#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

namespace ozone
{
    typedef uint64_t shmid_t;
    typedef uint64_t semid_t;
    typedef uint64_t pid_t;

    //typedef uid_t uint64_t;
    //typedef gid_t uint64_t;

    constexpr pid_t INVALID_PROCESS = 1024;
    constexpr semid_t INVALID_SEMAPHORE = 1024;
    constexpr shmid_t INVALID_SHAREDMEMORY = 1024;

    enum signal_t
    {
        SIGKILL, //forcefully kill (cannot be handled)
        SIGTERM, //request to close
        SIGSTOP, //pause (cannot be handled)
        SIGCONT, //resume (cannot be handled)
        SIGSEGV, //wrong memory access
        SIGILL,  //illegal instruction
        SIGINT,  //ctrl+c
        SIGUSR1, //customizable signal, if not set but called doesn't kill the process
        SIGUSR2, //customizable signal, if not set but called doesn't kill the process
    };

    struct terminal_size_t
    {
        uint16_t x,y;
    };

    namespace user
    {

        uint64_t sys_call_n(uint64_t sys_call_number, uint64_t arg0 = 0, uint64_t arg1 = 0, uint64_t arg2 = 0);

        //returns the id of the current process
        pid_t get_id();
        //stops the process for <ticks> ticks of clock
        void sleep(uint64_t ticks);
        //kills the current process
        void exit(uint64_t ret = 0);
        //used to return from a process
        extern "C" void fin();
        //allocates a semaphore to manage access to shared resources
        semid_t create_semaphore(int64_t start_count);
        //request access to a shared resource
        void acquire_semaphore(semid_t semaphore_id);
        //releases the access to a shared resource
        void release_semaphore(semid_t semaphore_id);
        //creates a child process which inherits the pagigng trie (stack excluded) and has entrypoint main
        //returns the process_id of the new process, user::exit() will be automatically called if the functions returns
        template <typename T>
        uint64_t fork(T (*main)())
        { //we should pass the address of user::exit()
            return sys_call_n(6, (uint64_t)main, (uint64_t)user::fin);
        }
        //waits for a process to end and returns its return
        uint64_t join(pid_t id);
        //calls a function from a driver returns the process id of the call, it's asyncronous so you can make a "join" call later to take the return
        pid_t driver_call(uint64_t driver_number, uint64_t function_number);
        //send a signal to a process, returns false if the process is not present
        bool signal(signal_t signal, pid_t process_id);
        //select a function
        void set_signal_handler(signal_t signal, void (*handler)());
        //called at the return of a signal handler (one of those set with set_signal_handler)
        void return_from_signal();
        //creates a shared memory (owner can always read and write, user can write only if user_rw is set) of a desired size (will be aligned to 4KiB)
        //returns the shared memory id
        shmid_t shm_get(bool user_rw, size_t size);
        //waits for every user to deatach from the shared memory and destroys it, if the process hasn't already called shm_detach it is called automaticcally
        void shm_wait_and_destroy(shmid_t shmem_id);
        //attaches a shared memory to the process, returns the starting address of said memory, if nullptr is returned the shared memory doesn't exist
        void* shm_attach(shmid_t shmem_id);
        //detaches a shared memory from a process, returns false if the shared memory doesn't exist
        bool shm_detach(shmid_t shmem_id);
        //requests the handle for the standard output, returns the size of the available terminal
        terminal_size_t get_stdout_handle();
        //releases the handle for the standard output
        void release_stdout_handle();
        //prints a char <c> at {x,y} of color <color> (32b bg & 32b fg)
        void put_char(char c,uint64_t color, uint16_t x, uint16_t y);
        //requests the handle for the standard input
        void get_stdin_handle();
        //releases the handle for the standard input
        void release_stdin_handle();
        //gets a char from kb
        char get_char();
    };

    namespace system
    {
        uint64_t sys_call_n(uint64_t sys_call_number, uint64_t arg0 = 0, uint64_t arg1 = 0, uint64_t arg2 = 0, uint64_t arg3 = 0);

        //sets the selected process as the driver for the selected irq
        void set_driver(uint64_t irq_number, pid_t process_id);
        //waits for the next interrupt associated with the process, returns the id of the interrupted process
        uint64_t wait_for_interrupt();

        void set_driver_function(uint64_t irq_number, uint64_t function_id, uint64_t (*function)());
    };
};

void *operator new(size_t size);
void *operator new[](size_t size);
//void* operator new(size_t size, size_t align);
void operator delete(void *address);
void operator delete[](void *address);
void operator delete(void *address, unsigned long);