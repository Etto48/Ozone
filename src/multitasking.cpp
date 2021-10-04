#include "include/multitasking.h"

namespace multitasking
{
    process_descriptor_t process_array[MAX_PROCESS_NUMBER];
    semaphore_descriptor_t semaphore_array[MAX_SEMAPHORE_NUMBER];
    shm_descriptor_t shm_array[MAX_SHAREDMEMORY_NUMBER];

    volatile ozone::pid_t execution_index = MAX_PROCESS_NUMBER; //for forcing the scheduler to initialize
    volatile uint64_t process_count = 0;

    process_descriptor_t *ready_queue = nullptr;
    process_descriptor_t *last_ready = nullptr;

    extern "C"
    {
        uint64_t sys_stack_base = system_stack_bottom_address;
    }

    void print_queue()
    {
        for (auto p = ready_queue; p; p = p->next)
        {
            printf("%ld ", p->id);
        }
        printf("\n");
    }

    void add_to_list(ozone::pid_t process_id, process_descriptor_t *&head, process_descriptor_t *&tail)
    {
        if (process_array[process_id].is_present)
        {
            auto p = &process_array[process_id];
            if (!tail)
            {
                head = p;
            }
            else
            {
                tail->next = p;
            }
            tail = p;
            p->next = nullptr;
        }
    }

    ozone::pid_t pop_from_list(process_descriptor_t *&head, process_descriptor_t *&tail)
    {
        if (!head)
            abort("Process requested but queue is empty");
        auto ret = head->id;
        if (head == tail)
            tail = nullptr;
        head = head->next;
        return ret;
    }

    ozone::pid_t pop_ready() //pop from top
    {
        //printf("POP    ");
        //print_queue();
        return pop_from_list(ready_queue, last_ready);
    }
    void add_ready(ozone::pid_t process_id) //add to tail
    {
        //debug::log(debug::level::inf,"ADD %uld ",process_id);
        //print_queue();
        add_to_list(process_id, ready_queue, last_ready);
    }

    ozone::pid_t get_available_index()
    {
        for (ozone::pid_t i = 0; i < MAX_PROCESS_NUMBER; i++)
        {
            if (!process_array[i].is_present)
            {
                return i;
            }
        }
        return MAX_PROCESS_NUMBER;
    }

    ozone::semid_t get_available_semaphore()
    {
        for (ozone::semid_t i = 0; i < MAX_SEMAPHORE_NUMBER; i++)
        {
            if (!semaphore_array[i].is_present)
            {
                return i;
            }
        }
        return MAX_SEMAPHORE_NUMBER;
    }

    ozone::shmid_t get_available_shmemory()
    {
        for (ozone::shmid_t i = 0; i < MAX_SHAREDMEMORY_NUMBER; i++)
        {
            if (!shm_array[i].is_present)
            {
                return i;
            }
        }
        return MAX_SHAREDMEMORY_NUMBER;
    }

    ozone::semid_t create_semaphore(int64_t starting_count)
    {
        auto id = get_available_semaphore();
        semaphore_array[id].is_present = true;
        semaphore_array[id].count = starting_count;
        semaphore_array[id].waiting_list_head = nullptr;
        semaphore_array[id].waiting_list_tail = nullptr;
        semaphore_array[id].creator_id = execution_index;
        return id;
    }

    void acquire_semaphore(ozone::semid_t semaphore_id)
    {
        if (semaphore_id < MAX_SEMAPHORE_NUMBER && semaphore_array[semaphore_id].is_present)
        {
            semaphore_array[semaphore_id].count--;
            if (semaphore_array[semaphore_id].count < 0) //move the current process into the waiting list
            {
                auto to_add = execution_index;
                drop();
                add_to_list(to_add, semaphore_array[semaphore_id].waiting_list_head, semaphore_array[semaphore_id].waiting_list_tail);
            }
        }
        else
        {
            abort("Semaphore no more present, maybe the creator process was closed?");
        }
    }
    void release_semaphore(ozone::semid_t semaphore_id)
    {
        if (semaphore_array[semaphore_id].is_present)
        {
            semaphore_array[semaphore_id].count++;
            if (semaphore_array[semaphore_id].count <= 0)
            {
                add_ready(pop_from_list(semaphore_array[semaphore_id].waiting_list_head, semaphore_array[semaphore_id].waiting_list_tail));
            }
        }
    }

    ozone::pid_t fork(ozone::pid_t process_id, void (*main)(), void (*fin)())
    {   if(!(is_process_memory((void*)main,1,process_id) && is_process_memory((void*)fin,1,process_id)))
        {
            abort("Access error");
        }
        if (process_array[process_id].is_present)
        {
            
            auto l4 = paging::table_alloc();
            auto &ol4 = process_array[process_id].paging_root;
            l4->copy_from(*ol4, 0, 1);
            for (mapping_history_t *p = process_array[process_id].mapping_history; p; p = p->next)
            {
                if (paging::map(
                        p->starting_address, p->npages, p->flags, l4, [ol4](void *vv, bool big_pages)
                        {
                            //debug::log(debug::level::wrn,"Frame mapped in copy");
                            auto l1 = ol4->operator[](paging::get_index_of_level(vv, 4))->operator[](paging::get_index_of_level(vv, 3))->operator[](paging::get_index_of_level(vv, 2));
                            return (void *)(big_pages ? l1 : l1->operator[](paging::get_index_of_level(vv, 1)));
                        },
                        p->flags & paging::flags::BIG) != (void *)0xffffffffffffffff)
                    abort("Error in memory copy");
            }

            return create_process((void *)main, l4, process_array[process_id].level, process_id, process_array[process_id].mapping_history, fin);
        }
        else
            return MAX_PROCESS_NUMBER;
    }

    void exit(uint64_t ret)
    {
        //debug::log(debug::level::inf, "Process %uld returned with code %uld", multitasking::execution_index, ret);
        while (process_array[execution_index].waiting_head)
        {
            auto join_process = pop_from_list(
                process_array[execution_index].waiting_head,
                process_array[execution_index].waiting_tail);
            process_array[join_process].context.rax = ret;
            add_ready(join_process);
        }
        destroy_process(multitasking::execution_index);
        drop(); //just to be sure
    }

    void kill(ozone::pid_t process_id)
    {
        while (process_array[process_id].waiting_head)
        {
            auto join_process = pop_from_list(
                process_array[process_id].waiting_head,
                process_array[process_id].waiting_tail);
            process_array[join_process].context.rax = 0;
            add_ready(join_process);
        }
        destroy_process(process_id);
    }

    uint64_t join(ozone::pid_t pid)
    {
        bool permission_accepted = false;
        if (process_array[execution_index].level == interrupt::privilege_level_t::system || process_array[pid].level == interrupt::privilege_level_t::user)
            permission_accepted = true;
        else
        {
            for (uint64_t i = 0; i < interrupt::IRQ_SIZE; i++)
            {
                if (interrupt::io_descriptor_array[i].is_present && process_array[interrupt::io_descriptor_array[i].id].is_present && process_array[pid].father_id == i)
                {
                    permission_accepted = true;
                    break;
                }
            }
        }
        if (permission_accepted)
        {
            if (process_array[pid].is_present)
            {
                add_to_list(execution_index, process_array[pid].waiting_head, process_array[pid].waiting_tail);
                drop();
                return 0;
            }
            else
            {
                return process_array[pid].context.rdx;
            }
        }
        else
            return 0;
    }

    ozone::pid_t driver_call(uint64_t driver_id, uint64_t function_id)
    {
        if (driver_id < interrupt::IRQ_SIZE && function_id < interrupt::MAX_DRIVER_FUNCTIONS)
        {
            auto &io_descriptor = interrupt::io_descriptor_array[driver_id];
            if (io_descriptor.is_present && io_descriptor.is_ready)
            {
                if (io_descriptor.function_array[function_id])
                {
                    if (process_array[io_descriptor.id].is_present)
                    {
                        return fork(io_descriptor.id, (void (*)())(io_descriptor.function_array[function_id]), io_descriptor.fin_address);
                    }
                }
            }
        }
        return MAX_PROCESS_NUMBER;
    }

    void call_signal_handler(ozone::pid_t process_id, void (*handler)())
    {
        if (process_id < MAX_PROCESS_NUMBER && process_array[process_id].is_present)
        {
            auto &process = process_array[process_id];
            if (!process.signal_descriptor.is_running_a_signal)
            {
                process.signal_descriptor.is_running_a_signal = true;
                process.signal_descriptor.saved_context = process.context;
                process.context.rip = (uint64_t)handler;
                auto old_trie = paging::get_current_trie();
                paging::set_current_trie(process.paging_root);

                process.context.rsp -= 8;
                *(void (**)())process.context.rsp = process.signal_descriptor.return_from_signal;

                paging::set_current_trie(old_trie);
            }
        }
    }

    bool signal(ozone::pid_t source_process, ozone::signal_t signal, ozone::pid_t target_process)
    {
        if (target_process < MAX_PROCESS_NUMBER && source_process < MAX_PROCESS_NUMBER && process_array[target_process].is_present && process_array[source_process].is_present && (process_array[source_process].level == interrupt::privilege_level_t::system || (process_array[target_process].level == interrupt::privilege_level_t::user && process_array[target_process].father_id == process_array[source_process].father_id)))
        {
            const char *signals[] = {
                "SIGKILL",
                "SIGTERM",
                "SIGSTOP",
                "SIGCONT",
                "SIGSEGV",
                "SIGILL",
                "SIGUSR1",
                "SIGUSR2"};
            bool do_kill = false;
            switch (signal)
            {
            case ozone::SIGKILL:
                do_kill = true;
                break;
            case ozone::SIGTERM:
                if (process_array[target_process].signal_descriptor.SIGTERM_handler)
                    call_signal_handler(target_process, process_array[target_process].signal_descriptor.SIGTERM_handler);
                else
                    do_kill = true;
                break;
            case ozone::SIGSTOP:
                //WIP
                break;
            case ozone::SIGCONT:
                //WIP
                break;
            case ozone::SIGSEGV:
                if (process_array[target_process].signal_descriptor.SIGSEGV_handler)
                    call_signal_handler(target_process, process_array[target_process].signal_descriptor.SIGSEGV_handler);
                else
                    do_kill = true;
                break;
            case ozone::SIGILL:
                if (process_array[target_process].signal_descriptor.SIGSEGV_handler)
                    call_signal_handler(target_process, process_array[target_process].signal_descriptor.SIGSEGV_handler);
                else
                    do_kill = true;
                break;
            case ozone::SIGINT:
                if (process_array[target_process].signal_descriptor.SIGINT_handler)
                    call_signal_handler(target_process, process_array[target_process].signal_descriptor.SIGINT_handler);
                else
                    do_kill = true;
                break;
            case ozone::SIGUSR1:
                if (process_array[target_process].signal_descriptor.SIGUSR1_handler)
                    call_signal_handler(target_process, process_array[target_process].signal_descriptor.SIGUSR2_handler);
                break;
            case ozone::SIGUSR2:
                if (process_array[target_process].signal_descriptor.SIGUSR2_handler)
                    call_signal_handler(target_process, process_array[target_process].signal_descriptor.SIGUSR2_handler);
                break;
            default:
                break;
            }
            if (do_kill)
            {
                //debug::log(debug::level::wrn, "Process %uld terminated, reason: %s", target_process, signals[signal]);
                kill(target_process);
            }
            return true;
        }
        else
            return false;
    }

    void set_signal_handler(ozone::pid_t process_id, ozone::signal_t signal, void (*handler)(), void (*return_from_signal)())
    {
        if(!(is_process_memory((void*)handler,1,process_id) && is_process_memory((void*)return_from_signal,1,process_id)))
        {
            abort("Access error");
        }
        if (process_id < MAX_PROCESS_NUMBER && process_array[process_id].is_present)
        {
            auto &process = process_array[process_id];
            process.signal_descriptor.return_from_signal = return_from_signal;
            switch (signal)
            {
            case ozone::SIGTERM:
                process.signal_descriptor.SIGTERM_handler = handler;
                break;
            case ozone::SIGSEGV:
                process.signal_descriptor.SIGSEGV_handler = handler;
                break;
            case ozone::SIGINT:
                process.signal_descriptor.SIGINT_handler = handler;
                break;
            case ozone::SIGILL:
                process.signal_descriptor.SIGILL_handler = handler;
                break;
            case ozone::SIGUSR1:
                process.signal_descriptor.SIGUSR1_handler = handler;
                break;
            case ozone::SIGUSR2:
                process.signal_descriptor.SIGUSR2_handler = handler;
                break;
            default:
                break;
            }
        }
    }

    void return_from_signal(ozone::pid_t process_id)
    {
        if (process_id < MAX_PROCESS_NUMBER && process_array[process_id].is_present)
        {
            process_array[process_id].signal_descriptor.is_running_a_signal = false;
            process_array[process_id].context = process_array[process_id].signal_descriptor.saved_context;
        }
    }

    ozone::shmid_t shm_get(ozone::pid_t owner_id, bool user_rw, size_t size)
    {
        size = (size_t)memory::align((void *)size, 0x1000);
        uint64_t npages = size / 0x1000;
        if (npages == 0)
            return MAX_SHAREDMEMORY_NUMBER;
        auto id = get_available_shmemory();
        shm_array[id].is_present = true;
        shm_array[id].orphan = false;
        shm_array[id].owner_id = owner_id;
        shm_array[id].npages = npages;
        shm_array[id].owner_flags;
        shm_array[id].users = 0;
        shm_array[id].owner_flags = paging::flags::PRESENT | paging::flags::RW | (process_array[owner_id].level == interrupt::privilege_level_t::user ? paging::flags::USER : 0);
        shm_array[id].user_flags = paging::flags::PRESENT | (user_rw ? paging::flags::RW : 0) | paging::flags::USER;
        shm_array[id].first_frame_index = paging::FRAME_COUNT;
        shm_array[id].waiting_head = nullptr;
        shm_array[id].waiting_tail = nullptr;
        for (uint64_t i = 0; i < npages; i++)
        {
            auto f = paging::get_frame_index(paging::frame_alloc());
            paging::frame_descriptors[f].next_free_frame_index = shm_array[id].first_frame_index;
            shm_array[id].first_frame_index = f;
        }
        return id;
    }

    void shm_destroy(ozone::shmid_t id)
    {
        shm_array[id].is_present = false;
        if (shm_array[id].users != 0)
            abort("Shmem with users attached was deleted");
        while (shm_array[id].first_frame_index != paging::FRAME_COUNT)
        {
            auto next = paging::frame_descriptors[shm_array[id].first_frame_index].next_free_frame_index;
            paging::frame_descriptors[shm_array[id].first_frame_index].next_free_frame_index = 0;
            paging::free_frame(paging::get_frame_address(shm_array[id].first_frame_index));
            shm_array[id].first_frame_index = next;
        }
        while (shm_array[id].waiting_head)
            add_ready(pop_from_list(shm_array[id].waiting_head, shm_array[id].waiting_tail));
    }

    void shm_wait_and_destroy(ozone::shmid_t id)
    {
        if (id < MAX_SHAREDMEMORY_NUMBER && shm_array[id].is_present)
        {
            if (shm_array[id].users == 0)
            {
                shm_destroy(id);
            }
            else
            {
                shm_detach(execution_index, id);
                add_to_list(execution_index, shm_array[id].waiting_head, shm_array[id].waiting_tail);
                drop();
            }
        }
    }

    struct shm_attacher
    {
        uint64_t frame_index;
        shm_attacher(shm_descriptor_t &shmd)
        {
            frame_index = shmd.first_frame_index;
        }
        void *operator()(void *va, bool)
        {
            if (frame_index == paging::FRAME_COUNT)
                abort("Fatal error during shared memory mapping");
            auto ret = paging::get_frame_address(frame_index);
            frame_index = paging::frame_descriptors[frame_index].next_free_frame_index;
            return ret;
        }
    };
    void *shm_attach(ozone::pid_t source_process, ozone::shmid_t id)
    {
        if (id < MAX_SHAREDMEMORY_NUMBER && shm_array[id].is_present)
        {
            if (source_process < MAX_PROCESS_NUMBER && process_array[source_process].is_present)
            {
                for (auto p = process_array[source_process].shmem_history; p; p = p->next) //check if already attached
                {
                    if (p->id == id)
                        return p->starting_address;
                }
                //debug::log(debug::level::inf, "Shmem %uld attached to process %uld", id, source_process);
                shm_array[id].users++;

                auto flags = shm_array[id].user_flags;
                if (source_process == shm_array[id].owner_id)
                {
                    flags = shm_array[id].owner_flags;
                }

                uint64_t starting_address = 0xffffffff00000000;
                //now we must calculate the starting address
                if (process_array[source_process].shmem_history)
                {
                    starting_address = (uint64_t)process_array[source_process].shmem_history->starting_address;
                    starting_address += (shm_array[process_array[source_process].shmem_history->id].npages + 1) * 0x1000;
                }

                if (paging::map((void *)starting_address, shm_array[id].npages, flags, process_array[source_process].paging_root, shm_attacher{shm_array[id]}, false) != (void *)0xffffffffffffffff)
                    abort("Error after mapping");

                auto new_shm_hystory = (shmem_attaching_history_t *)system_heap.malloc(sizeof(shmem_attaching_history_t));
                new_shm_hystory->id = id;
                new_shm_hystory->starting_address = (void *)starting_address;
                new_shm_hystory->next = process_array[source_process].shmem_history;
                process_array[source_process].shmem_history = new_shm_hystory;
                return process_array[source_process].shmem_history->starting_address;
            }
        }
        return nullptr;
    }

    bool shm_detach(ozone::pid_t source_process, ozone::shmid_t id)
    {
        if (id < MAX_SHAREDMEMORY_NUMBER && shm_array[id].is_present)
        {
            if (source_process < MAX_PROCESS_NUMBER && process_array[source_process].is_present)
            {
                shmem_attaching_history_t *last = nullptr;
                auto p = process_array[source_process].shmem_history;
                for (; p && p->id != id; p = p->next) //check if already attached
                {
                    last = p;
                }
                if (p) //we found it
                {
                    //debug::log(debug::level::inf, "Shmem %uld detached from process %uld", id, source_process);
                    shmem_attaching_history_t *to_remove = p;
                    if (!last) //head
                    {
                        process_array[source_process].shmem_history = to_remove->next;
                    }
                    else
                    {
                        last->next = to_remove->next;
                    }
                    auto unm = paging::unmap(
                        to_remove->starting_address, shm_array[to_remove->id].npages, process_array[source_process].paging_root, [](void *, bool) {}, false);
                    if (unm != (void *)0xffffffffffffffff)
                        abort("Error after unmapppinng");
                    system_heap.free(to_remove);
                    shm_array[id].users--;

                    if (shm_array[id].users == 0 && (shm_array[id].waiting_head || shm_array[id].orphan)) //if we requested a destruction we must accomplish that
                        shm_destroy(id);
                    return true;
                }
            }
        }
        return false;
    }

    bool add_pages(ozone::pid_t target_process,void* page_address,uint8_t flags,uint64_t page_count)
    {
        if(target_process < MAX_PROCESS_NUMBER && process_array[target_process].is_present)
        {
            if(paging::map(page_address,page_count,flags,process_array[target_process].paging_root,[](void*,bool){return paging::frame_alloc();},false)==(void*)0xffffffffffffffff)
            {//success
                auto new_history = (mapping_history_t*)system_heap.malloc(sizeof(mapping_history_t));
                new_history->flags = flags;
                new_history->npages = page_count;
                new_history->starting_address = page_address;
                new_history->next = process_array[target_process].private_mapping_history;
                process_array[target_process].private_mapping_history = new_history;
                return true;
            }
        }
        return false;
    }

    void init_process_array()
    {
        for (ozone::pid_t i = 0; i < MAX_PROCESS_NUMBER; i++)
        {
            process_array[i].id = i;
        }
    }

    void *create_stack(paging::page_table_t *paging_root, interrupt::privilege_level_t privilege_level)
    {
        if (privilege_level == interrupt::privilege_level_t::user)
        {
            uint16_t flags = paging::flags::RW | paging::flags::USER;
            if (paging::map((void *)stack_top_address, stack_pages, flags, paging_root, [](void *virtual_address, bool big_pages)
                            { return paging::frame_alloc(); },
                            false) != (void *)0xffffffffffffffff)
                abort("Process user stack not created");
        }
        uint16_t sys_flags = paging::flags::RW;
        if (paging::map((void *)system_stack_top_address, stack_pages, sys_flags, paging_root, [](void *virtual_address, bool big_pages)
                        { return paging::frame_alloc(); },
                        false) != (void *)0xffffffffffffffff)
            abort("Process system stack not created");
        return (void *)(privilege_level == interrupt::privilege_level_t::user ? stack_bottom_address : system_stack_bottom_address);
    }
    void *create_stack(ozone::pid_t process_id, interrupt::privilege_level_t privilege_level)
    {
        if (privilege_level == interrupt::privilege_level_t::user)
        {
            uint16_t flags = paging::flags::RW | paging::flags::USER;
            if(!add_pages(process_id,(void*)(stack_bottom_address&~0xfff),flags,1))
                abort("Stack creation error");
        }
        uint16_t sys_flags = paging::flags::RW;
        if(!add_pages(process_id,(void*)system_stack_top_address,sys_flags,system_stack_pages))
            abort("Stack creation error");
        return (void *)(privilege_level == interrupt::privilege_level_t::user ? stack_bottom_address : system_stack_bottom_address);
    }
    void *create_heap(ozone::pid_t process_id, interrupt::privilege_level_t privilege_level)
    {
        uint16_t flags = privilege_level == interrupt::privilege_level_t::user? (paging::flags::RW | paging::flags::USER) : (flags = paging::flags::RW);
        if(!add_pages(process_id,(void*)heap_top_address,flags,heap_pages))
            abort("Heap creation error");

        return (void *)(heap_top_address);
    }
    void destroy_stack(paging::page_table_t *paging_root)
    {
        paging::unmap((void *)stack_top_address, stack_pages, paging_root, [](void *phisical_address, bool big_pages)
                      { paging::free_frame(phisical_address); },
                      false);
        paging::unmap((void *)system_stack_top_address, stack_pages, paging_root, [](void *phisical_address, bool big_pages)
                      { paging::free_frame(phisical_address); },
                      false);
    }

    ozone::pid_t create_process(void *entrypoint, paging::page_table_t *paging_root, interrupt::privilege_level_t level, ozone::pid_t father_id, mapping_history_t *mapping_history, void (*fin)())
    {
        if (process_count < MAX_PROCESS_NUMBER)
        {
            uint64_t process_id = get_available_index();
            process_array[process_id].is_present = true;
            //auto translated_entry = paging::virtual_to_phisical(entrypoint,paging_root);//for debug

            process_array[process_id].id = process_id;
            process_array[process_id].father_id = father_id;
            process_array[process_id].level = level;
            process_array[process_id].paging_root = paging_root;
            
            process_array[process_id].waiting_head = process_array[process_id].waiting_tail = nullptr;
            process_array[process_id].signal_descriptor = signal_handling_descriptor_t{};

            process_array[process_id].shmem_history = nullptr; //init without shm mappings, they are not inherited
            process_array[process_id].mapping_history = mapping_history;
            process_array[process_id].private_mapping_history = nullptr;

            auto stack_base = (void *)((uint64_t)create_stack(process_id, level) - 16);

            auto last_trie = paging::get_current_trie();
            paging::set_current_trie(paging_root);
            //interrupt::context_t* context = (interrupt::context_t*)((uint64_t)stack_base-sizeof(interrupt::context_t));
            interrupt::context_t context;
            context.ss = level == interrupt::privilege_level_t::user ? interrupt::GDT_USER_DATA_SEGMENT : 0;
            context.cs = level == interrupt::privilege_level_t::user ? interrupt::GDT_USER_CODE_SEGMENT : interrupt::GDT_SYSTEM_CODE_SEGMENT;
            constexpr uint64_t IF = 0x0200;
            constexpr uint64_t IOPL = 0x3000;
            context.rflags = level == interrupt::privilege_level_t::user ? IF : IF | IOPL;
            context.rip = (uint64_t)entrypoint;
            context.rsp = (uint64_t)stack_base;
            context.rbp = (uint64_t)stack_base;
            context.rax = context.rbx = context.rcx = context.rdx =
                context.rsi = context.rdi =
                    context.r8 = context.r9 = context.r10 = context.r11 = context.r12 = context.r13 = context.r14 = context.r15 = context.fs = context.gs = context.int_num = context.int_info = 0;

            //tricks the function to call automatically user::exit() it reaches the end (it can cause a crash in user code we need to fix this later)
            *(void **)((uint64_t)stack_base) = (void *)fin;

            process_array[process_id].context = context;
            
            process_array[process_id].process_heap = heap{create_heap(process_id,level), heap_pages * 0x1000};//1MiB
            process_array[process_id].next = nullptr;

            paging::set_current_trie(last_trie);
            //debug::log(debug::level::inf, "Process %uld created, level: %s, father_id: %uld", process_id, level == interrupt::privilege_level_t::user ? "user" : "system", father_id);
            process_count++;
            add_ready(process_id);
            return process_id;
        }
        else
            return MAX_PROCESS_NUMBER;
    }

    uint64_t remove_not_present(process_descriptor_t *&head, process_descriptor_t *&tail)
    {
        uint64_t removed = 0;
        process_descriptor_t *last = nullptr;
        process_descriptor_t *p;
        for (p = head; p; p = p->next)
        {
            if (!p->is_present) //we need to remove this from the list
            {
                if (!last) //head
                {
                    if (head == tail)
                        head = tail = nullptr;
                    else
                        head = p->next;
                }
                else
                {
                    if (!p->next)
                    {
                        last->next = nullptr;
                        tail = last;
                    }
                    else
                    {
                        last->next = p->next;
                    }
                }
                removed++;
            }
            else
            {
                last = p;
            }
        }
        return removed;
    }

    void destroy_semaphore(ozone::semid_t id)
    {
        semaphore_array[id].is_present = false;

        while (semaphore_array[id].waiting_list_head)
        {
            add_ready(pop_from_list(semaphore_array[id].waiting_list_head, semaphore_array[id].waiting_list_tail));
        }
    }
    void destroy_process(ozone::pid_t id)
    {
        if (process_array[id].is_present)
        {
            paging::set_current_trie(&paging::identity_l4_table);
            //destroy_stack(process_array[id].paging_root);

            while (process_array[id].shmem_history)
            {
                if (shm_array[process_array[id].shmem_history->id].owner_id == id)
                {
                    shm_array[process_array[id].shmem_history->id].orphan = true;
                    shm_array[process_array[id].shmem_history->id].owner_id = MAX_PROCESS_NUMBER;
                }
                shm_detach(id, process_array[id].shmem_history->id);
            }

            while (process_array[id].private_mapping_history)
            {
                auto to_remove = process_array[id].private_mapping_history;
                process_array[id].private_mapping_history = process_array[id].private_mapping_history->next;
                paging::unmap(to_remove->starting_address,to_remove->npages,process_array[id].paging_root,[](void* pa,bool){paging::free_frame(pa);},false);
                system_heap.free(to_remove);
            }

            if (process_array[id].father_id == MAX_PROCESS_NUMBER)
            {
                while (process_array[id].mapping_history)
                {
                    auto to_delete = process_array[id].mapping_history;
                    process_array[id].mapping_history = process_array[id].mapping_history->next;

                    paging::unmap(
                        to_delete->starting_address, to_delete->npages, process_array[id].paging_root, [](void *phisical_address, bool big_pages)
                        { return paging::free_frame(phisical_address); },
                        false);
                    system_heap.free(to_delete);
                }
            }
            else
            {
                for (mapping_history_t *p = process_array[id].mapping_history; p; p = p->next)
                {
                    paging::unmap(
                        p->starting_address, p->npages, process_array[id].paging_root, [](void *, bool) {}, p->flags & paging::flags::BIG);
                }
            }
            paging::destroy_paging_trie(process_array[id].paging_root);
            process_count--;
            for (uint64_t i = 0; i < MAX_SEMAPHORE_NUMBER; i++)
            {
                if (semaphore_array[i].is_present && semaphore_array[i].creator_id == execution_index)
                {
                    destroy_semaphore(i);
                }
            }
            for (uint64_t i = 0; i < MAX_PROCESS_NUMBER; i++) //kill the younglings (child processes)
            {
                if (process_array[i].is_present && process_array[i].father_id == id)
                {
                    destroy_process(i);
                }
            }
            process_array[id].is_present = false;
            //check if the process is present in some list, in case remove it
            remove_not_present(ready_queue, last_ready);
            for (uint64_t i = 0; i < MAX_SEMAPHORE_NUMBER; i++)
            {
                if (semaphore_array[i].is_present)
                {
                    auto removed_asking = remove_not_present(semaphore_array[i].waiting_list_head, semaphore_array[i].waiting_list_tail);
                    semaphore_array[i].count += removed_asking;
                }
            }
            clock::clean_timer_list();

            //debug::log(debug::level::inf, "Process %uld destroyed", id);
        }
    }

    bool is_process_memory(void *start, size_t len, ozone::pid_t id)
    {
        uint64_t s = (uint64_t)start;
        if (!process_array[id].is_present)
            return false;
        else if (!memory::is_normalized(start))
            return false;
        else if (process_array[id].level == interrupt::privilege_level_t::system)
            return true;
        else
        {
            auto offset = s & 0xfff;
            auto l = len + offset;
            s = s & ~0xfff;
            auto num_pages = l % 0x1000 == 0 ? (l / 0x1000) : (l / 0x1000 + 1);

            for (uint64_t i = 0; i < num_pages; i++)
            {
                if (!paging::virtual_to_phisical((void *)(s + i * 0x1000), process_array[id].paging_root))
                    return false;
                else if (paging::get_level((void *)(s + i * 0x1000), process_array[id].paging_root) != interrupt::privilege_level_t::user)
                    return false;
            }
            return true;
        }
    }

    volatile uint64_t scheduler_timer_ticks = 0;
    ozone::pid_t next_present_process()
    {
        ozone::pid_t id;
        do
        {
            id = pop_ready();
        } while (!process_array[id].is_present);
        return id;
    }
    void scheduler()
    {
        if (execution_index < MAX_PROCESS_NUMBER)
        {
            if (scheduler_timer_ticks >= timesharing_interval || (!process_array[execution_index].is_present))
            {
                next();
            }
        }
        else
        {
            execution_index = 0; //initialization
            drop();
        }
    }
    void drop()
    {
        scheduler_timer_ticks = 0;
        //debug::log(debug::level::inf,"process dropped");
        execution_index = next_present_process();
    }
    void next()
    {
        scheduler_timer_ticks = 0;
        auto last_exec = execution_index;
        if (ready_queue)
        {
            execution_index = next_present_process();
            add_ready(last_exec);
            //debug::log(debug::level::inf,"process swapped");
        }
    }

    void log_panic(const char *message, interrupt::context_t *context = nullptr)
    {
        if (!context)
            context = &process_array[execution_index].context;
        debug::log(debug::level::err, "----------------------------------------");
        if(execution_index==MAX_PROCESS_NUMBER)
            debug::log(debug::level::err, "System crashed");
        else
            debug::log(debug::level::err, "Process %uld crashed", execution_index);
        if (message)
            debug::log(debug::level::err, "MSG: %s", message);
        if (context->int_num < 32)
            debug::log(debug::level::err, "Cause: %s (0x%x)", interrupt::isr_messages[context->int_num], context->int_info);
        else if (context->int_num >= 32 && context->int_num < 32 + 24)
            debug::log(debug::level::err, "Cause: Error during IRQ number %uld", context->int_num - 32);
        else if (context->int_num == 0x80)
            debug::log(debug::level::err, "Cause: Error during syscall %s", syscalls_names[context->rsi]);
        else
            debug::log(debug::level::err, "Cause: INT %uld (%uld)", context->int_num, context->int_info);
        debug::log(debug::level::err, "RIP: 0x%p", context->rip);
        if (context->int_num == 14)
        { //page fault
            debug::log(debug::level::err, "Accessed address: 0x%p", paging::get_cr2());
        }
        debug::log(debug::level::err, "CS:0x%p SS:0x%p", context->cs, context->ss);
        debug::log(debug::level::err, "Stack Pointer: 0x%p", context->rsp);
        debug::log(debug::level::err, "Base pointer: 0x%p", context->rbp);
        debug::log(debug::level::err, "----------------------------------------");
    }
    void panic(const char *message, interrupt::context_t *context)
    {
        if (!context)
            context = &process_array[execution_index].context;
        clear(0x4f);
        video::draw_image(ozone_panic_logo[0],{256,256},video::get_screen_size()-video::v2i{256,256});
        printf("\e[48;5;15m\e[31mKERNEL PANIC\n\e[38;5;15m\e[41m");
        if (message)
            printf("MSG: %s\n", message);
        if(execution_index!=MAX_PROCESS_NUMBER)
            printf("Process id: %uld\n", execution_index);
        else
            printf("System error\n");
        printf("Cause: ");
        if (context->int_num < 32)
            printf("%s (0x%x)", interrupt::isr_messages[context->int_num], context->int_info);
        else if (context->int_num >= 32 && context->int_num < 32 + 24)
            printf("Error during IRQ number %uld", context->int_num - 32);
        else if (context->int_num == 0x80)
            printf("Error during user syscall %s", syscalls_names[context->rsi]);
        else if (context->int_num == 0x81)
            printf("Error during system syscall %uld", context->rsi);
        else if (context->int_num == 0x82)
            printf("Error during driver call d:%uld f:%uld", context->rsi, context->rdx);
        else
            printf("INT %uld (%uld)", context->int_num, context->int_info);
        printf("\nRIP: 0x%p\n", context->rip);
        if (context->int_num == 14)
        { //page fault
            printf("Accessed address: 0x%p\n", paging::get_cr2());
            printf("Error flags: ");
            if (context->int_info & (1 << 0))
                printf("[Present] ");
            if (context->int_info & (1 << 1))
                printf("[Write] ");
            if (context->int_info & (1 << 2))
                printf("[User] ");
            if (context->int_info & (1 << 3))
                printf("[Reserved write] ");
            if (context->int_info & (1 << 4))
                printf("[Instruction fetch]");
            printf("\n");
        }
        printf("\nFlags: 0b%b\nCS: 0x%p\n", context->rflags, context->cs);
        printf("RAX:0x%p RBX:0x%p\nRCX:0x%p RDX:0x%p\nRSI:0x%p RDI:0x%p\n", context->rax, context->rbx, context->rcx, context->rdx, context->rsi, context->rdi);
        printf("Stack segment: 0x%p\nStack Pointer: 0x%p\nBase pointer: 0x%p\n", context->ss, context->rsp, context->rbp);

        while(true)
            asm volatile("hlt");
    }

    void abort(const char *msg, interrupt::context_t *context)
    {
        if (execution_index >= MAX_PROCESS_NUMBER || process_array[execution_index].level == interrupt::privilege_level_t::system || force_panic)
        {
            debug::log(debug::level::err, "KERNEL PANIC");
            log_panic(msg, context);
            panic(msg, context);
        }
        else
        {
            /*if(process_array[execution_index].context->int_num<interrupt::ISR_SIZE)
                printf("\033c\x0cProcess %ld aborted, reason: %s ", execution_index, interrupt::isr_messages[process_array[execution_index].context->int_num]);
            else
                printf("\033c\x0cProcess %ld aborted ", execution_index);
            if(msg)
                printf("message: %s",msg);
            printf("\n");*/
            log_panic(msg);
            destroy_process(execution_index);
        }
    }
    void save_state(interrupt::context_t *context)
    {
        //current process is execution_index if it's valid
        if (execution_index < MAX_PROCESS_NUMBER)
        { //save the current state to the process descriptor
            process_array[execution_index].context = *context;
        }
    }
    interrupt::context_t *load_state()
    {
        //we need to call the scheduler to update execution_index
        scheduler();
        //once we know which process to run, update cr3 and stack pointer, the rest of the context
        //will be automatically restored before iret
        paging::set_current_trie(process_array[execution_index].paging_root); //change cr3

        return &process_array[execution_index].context;
    }

};