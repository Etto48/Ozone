#pragma once
#include "interrupt.h"
#include "paging.h"
#include "heap.h"
#include <ozone.h>
#include "ozone_panic_logo.h"

namespace multitasking
{
    /*enum class process_priority_t
    {
        idle, normal
    };*/
    //contains the data used to call and return from a signal
    struct signal_handling_descriptor_t
    {
        bool is_running_a_signal = false;
        interrupt::context_t saved_context;
        void(*SIGTERM_handler)() = nullptr;
        void(*SIGSEGV_handler)() = nullptr;
        void(*SIGILL_handler)() = nullptr;
        void(*SIGINT_handler)() = nullptr;
        void(*SIGUSR1_handler)() = nullptr;
        void(*SIGUSR2_handler)() = nullptr;

        void(*return_from_signal)() = nullptr;
    };

    //describes one memory mapping containing the process code and data (those are done at the start of the process)
    struct mapping_history_t
    {
        void* starting_address;
        uint16_t flags;
        uint64_t npages;

        mapping_history_t* next = nullptr;
    };

    struct shmem_attaching_history_t
    {
        void* starting_address;
        ozone::shmid_t id = ozone::INVALID_SHAREDMEMORY;

        shmem_attaching_history_t* next = nullptr;
    };

    

    //contains the state of a process
    struct process_descriptor_t
    {
        bool is_present = false;
        ozone::pid_t id;
        ozone::pid_t father_id;
        interrupt::privilege_level_t level;
        interrupt::context_t context;
        paging::page_table_t *paging_root; //cr3 register

        shmem_attaching_history_t* shmem_history = nullptr;//we save every call to shmat()
        mapping_history_t* mapping_history = nullptr; //we save every call of map() needed to allocate the code+data memory of the program, so we can later unmap
        mapping_history_t* private_mapping_history = nullptr;
        heap process_heap;

        //used for the join function
        process_descriptor_t *waiting_head = nullptr;
        process_descriptor_t *waiting_tail = nullptr;

        //used for signaling
        signal_handling_descriptor_t signal_descriptor;

        process_descriptor_t *next = nullptr;
    };
    
    //describes a shared memory unit, has every frame memorized in a linked list
    struct shm_descriptor_t
    {
        bool is_present = false;
        bool orphan = false;
        uint64_t npages;
        ozone::pid_t owner_id;
        uint64_t users = 0;
        
        uint16_t owner_flags = 0;
        uint16_t user_flags = 0;

        process_descriptor_t* waiting_head = nullptr;
        process_descriptor_t* waiting_tail = nullptr;

        uint64_t first_frame_index = paging::FRAME_COUNT;
    };
    
    struct semaphore_descriptor_t
    {
        bool is_present = false;
        int64_t count;

        ozone::pid_t creator_id;

        process_descriptor_t *waiting_list_head = nullptr;
        process_descriptor_t *waiting_list_tail = nullptr;
    };

    struct tss_t
    {
        uint32_t reserved0;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;
        uint64_t reserved1;
        uint64_t ist1;
        uint64_t ist2;
        uint64_t ist3;
        uint64_t ist4;
        uint64_t ist5;
        uint64_t ist6;
        uint64_t ist7;
        uint64_t reserved2;
        uint16_t reserved3;
        uint16_t iopb_offset;
    } __attribute__((packed));

    

    extern volatile uint64_t scheduler_timer_ticks;
    constexpr uint64_t timesharing_interval = 15;

    constexpr uint64_t stack_pages = 512 * 10; //max stack size
    constexpr uint64_t system_stack_pages = 64;
    constexpr uint64_t heap_pages = 128;
    constexpr uint64_t stack_bottom_address = 0xffffffffffffffff;
    constexpr uint64_t stack_top_address = stack_bottom_address - (stack_pages * 0x1000 - 1);

    constexpr uint64_t system_stack_bottom_address = stack_top_address - 1 - 0x1000;//at least one page of distance
    constexpr uint64_t system_stack_top_address = system_stack_bottom_address - (system_stack_pages * 0x1000 - 1);

    constexpr uint64_t heap_bottom_address = system_stack_top_address - 1 - 0x1000;
    constexpr uint64_t heap_top_address = heap_bottom_address - (heap_pages * 0x1000 -1);

    constexpr ozone::pid_t MAX_PROCESS_NUMBER = ozone::INVALID_PROCESS;
    constexpr ozone::semid_t MAX_SEMAPHORE_NUMBER = ozone::INVALID_SEMAPHORE;
    constexpr ozone::shmid_t MAX_SHAREDMEMORY_NUMBER = ozone::INVALID_SHAREDMEMORY;

    constexpr bool force_panic = false;

    extern process_descriptor_t process_array[MAX_PROCESS_NUMBER];
    extern semaphore_descriptor_t semaphore_array[MAX_SEMAPHORE_NUMBER];
    extern shm_descriptor_t shm_array[MAX_SHAREDMEMORY_NUMBER];

    extern volatile ozone::pid_t execution_index;
    extern volatile uint64_t process_count;

    extern process_descriptor_t *ready_queue;
    extern process_descriptor_t *last_ready;

    extern const char *syscalls_names[];

    extern "C" tss_t tss;

    extern "C" uint64_t sys_stack_location;

    ozone::pid_t get_available_index();
    ozone::semid_t get_available_semaphore();
    ozone::shmid_t get_available_shmemory();

    ozone::semid_t create_semaphore(int64_t starting_count);
    void acquire_semaphore(ozone::semid_t semaphore_id);
    void release_semaphore(ozone::semid_t semaphore_id);

    //fork syscall helper
    uint64_t fork(uint64_t process_id, void (*main)(), void (*exit)());

    //exit syscall helper
    void exit(uint64_t ret);

    //kill a process (SIGKILL)
    void kill(ozone::pid_t process_id);

    //used by signal to prepare the process for the handler
    void call_signal_handler(ozone::pid_t process_id, void(*handler)());

    //join syscall helper
    uint64_t join(ozone::pid_t pid);

    //driver_call syscall helper
    ozone::pid_t driver_call(uint64_t driver_id, uint64_t function_id);

    //signal syscall helper
    bool signal(ozone::pid_t source_process, ozone::signal_t signal, ozone::pid_t target_process);

    //set_signal_handler syscall helper
    void set_signal_handler(ozone::pid_t process_id, ozone::signal_t signal,void(*handler)(), void(*return_from_signal)());

    //return_from_signal syscall helper
    void return_from_signal(ozone::pid_t process_id);

    //shm_get syscall helper
    ozone::shmid_t shm_get(ozone::pid_t owner_id, bool user_rw, size_t size);

    //shm_destroy syscall helper
    void shm_wait_and_destroy(ozone::shmid_t id);

    //shm_attach
    void* shm_attach(ozone::pid_t source_process, ozone::shmid_t id);

    //shm_detach
    bool shm_detach(ozone::pid_t source_process, ozone::shmid_t id);

    //used for extending the stack returns false if failed
    bool add_pages(ozone::pid_t target_process,void* page_address,uint8_t flags,uint64_t page_count);

    void init_process_array();

    ozone::pid_t pop_ready();
    void add_ready(ozone::pid_t process_id);

    void *create_stack(paging::page_table_t *paging_root);
    void destroy_stack(paging::page_table_t *paging_root);

    ozone::pid_t create_process(void *entrypoint, paging::page_table_t *paging_root, interrupt::privilege_level_t level, ozone::pid_t father_id, mapping_history_t* mapping_history,void (*fin)() = ozone::user::fin);
    void destroy_process(ozone::pid_t id);

    //returns true if the selected memory space is accessible by the process, false if it's not
    bool is_process_memory(void *start, size_t len, ozone::pid_t id);

    //updates execution_index with the index of the next process to run
    void scheduler();
    //if the current process is no more ready for some reason, a call to this function will exclude if from scheduling
    void drop();
    //executes the next process if there is one
    void next();

    //the current process caused a panic, system halted
    void panic(const char *message = nullptr, interrupt::context_t* context = nullptr);
    //kills the current process because an error occurred
    void abort(const char *msg = nullptr, interrupt::context_t* context = nullptr);
    //saves context about the current running process
    void save_state(interrupt::context_t *context);
    //runs the scheduler, changes cr3 and returns the stack index
    interrupt::context_t *load_state();
    //disable interrupts
    void cli();
    //enables interrupts
    void sti();

};
#include "clock.h"
#include "debug.h"
