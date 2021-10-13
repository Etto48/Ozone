#include "include/clock.h"

namespace clock
{
    constexpr uint16_t COMMAND_PORT = 0x43;
    constexpr uint16_t CLOCK_0 = 0x40;
    constexpr uint16_t CLOCK_1 = 0x41;
    constexpr uint16_t CLOCK_2 = 0x42;

    volatile uint64_t clock_ticks = 0;

    struct process_timer_t
    {
        ozone::pid_t id;
        uint64_t ticks;

        process_timer_t *next;
    };

    process_timer_t *timer_list = nullptr;

    void set_freq(uint64_t hz)
    {
        int divisor = 1193180 / hz;        /* Calculate our divisor */
        io::outb(COMMAND_PORT, 0x36);      /* Set our command byte 0x36 */
        io::outb(CLOCK_0, divisor & 0xFF); /* Set low byte of divisor */
        io::outb(CLOCK_0, divisor >> 8);   /* Set high byte of divisor */
    }
    void init()
    {
        set_freq(1000);
        interrupt::irq_callbacks[0] = callback;
    }

    void print_list()
    {
        for (auto p = timer_list; p; p = p->next)
        {
            printf("%uld(%uld) ", p->id, p->ticks);
        }
        printf("\n");
    }

    void execute_timer_recursive(process_timer_t *&list)
    {
        if (list && list->ticks == 0)
        {
            //multitasking::scheduler_timer_ticks = 0;
            auto to_free = list;
            multitasking::add_ready(list->id);
            list = list->next;
            execute_timer_recursive(list);
            system_heap.free(to_free);
        }
    }

    void callback(interrupt::context_t *context)
    {
        //print_list();
        multitasking::scheduler_timer_ticks++;
        clock_ticks++;
        if (timer_list)
        {
            timer_list->ticks--;
            /*while(timer_list && timer_list->ticks==0)
            {//run and extract
                multitasking::add_ready(timer_list->id);
                auto to_remove = timer_list;
                timer_list = timer_list->next;
                system_heap.free(to_remove);
            }*/
            execute_timer_recursive(timer_list);
        }
    }

    void add_timer_recursive(ozone::pid_t id, uint64_t ticks, process_timer_t *&list)
    {
        if ((!list) || (ticks < list->ticks))
        {
            auto old = list;
            auto new_timer = (process_timer_t *)system_heap.malloc(sizeof(process_timer_t));
            list = new_timer;
            new_timer->id = id;
            new_timer->ticks = ticks;
            new_timer->next = old;
            if (old)
            {
                old->ticks -= ticks;
            }
        }
        else
        {
            add_timer_recursive(id, ticks - list->ticks, list->next);
        }
    }

    uint64_t clean_timer_list()
    {
        uint64_t removed = 0;
        process_timer_t *last = nullptr;
        process_timer_t *p;
        process_timer_t *to_free = nullptr;
        for (p = timer_list; p; p = p->next)
        {
            if (to_free)
            {
                system_heap.free(to_free);
                to_free = nullptr;
            }
            if (!multitasking::process_array[p->id].is_present)
            {
                if (!last)
                {
                    timer_list = p->next;
                }
                else
                {
                    last->next = p->next;
                }
                if (p->next)
                {
                    p->next->ticks += p->ticks;
                }
                to_free = p;
                removed++;
            }
            else
            {
                last = p;
            }
        }
        return removed;
    }

    void add_timer(uint64_t ticks)
    {
        process_timer_t *last = nullptr;
        process_timer_t *p;
        for (
            p = timer_list;
            p && p->ticks < ticks;
            ticks -= p->ticks,
           p = p->next)
        {
            last = p;
        }
        process_timer_t *new_process_timer = (process_timer_t *)system_heap.malloc(sizeof(process_timer_t));
        new_process_timer->id = multitasking::current_execution_index();
        new_process_timer->next = p;
        new_process_timer->ticks = ticks;
        if (p)
        {
            p->ticks -= new_process_timer->ticks;
        }
        if (!last) //head
        {
            timer_list = new_process_timer;
        }
        else //normal
        {
            last->next = new_process_timer;
        }
        //printf("ADDED %uld(%uld)\n",multitasking::execution_index,ticks);
        //add_timer_recursive(multitasking::execution_index,ticks,timer_list);
        //print_list();
        multitasking::drop();
    }
    void mwait(uint64_t milliseconds)
    {
        auto target = clock_ticks+milliseconds;
        while(clock_ticks<target);
    }
};