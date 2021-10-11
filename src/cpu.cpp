#include "include/cpu.h"

namespace cpu
{
    uint8_t bspdone = 0;
    volatile uint8_t aprunning = 0;

    cpu_descriptor_t cpu_array[MAX_CPU_COUNT];

    void init()
    {
        auto cpus = get_count();
        debug::log(debug::level::inf,"%uld CPU cores found",cpus);
        printf("%uld CPU cores found\n",cpus);
    }
    uint64_t get_count()
    {
        uint64_t ret = 0;
        for(auto& x: cpu_array)
        {
            if(x.is_present)
                ret++;
        }
        return ret;
    }
    void start_cores()
    {
        multitasking::cli();
        aprunning = 0;
        bspdone = 0;
        //copy ap_trampoline to destination address
        memory::memcpy((void*)0x8000,(void*)&ap_trampoline,4096);
        auto bspid = apic::get_bspid();

        debug::log(debug::level::inf,"-Starting %ud more CPU cores",cpu::get_count()-1);
        printf("-Starting %ud more CPU cores\n",cpu::get_count()-1);
        for(auto& x: cpu_array)
        {
            if(x.is_present)
            {
                if(x.lapic_id==bspid) continue;
                
                apic::send_INIT(x.processor_id);
                multitasking::sti();
                clock::mwait(10);
                multitasking::cli();
                for(uint8_t j=0;j<2;j++)
                {
                    apic::send_SIPI(x.processor_id,0x000608);
                }
            }
        }
        bspdone = 1;
        multitasking::sti();
        clock::mwait(100);
        if(cpu::get_count()-1==aprunning)
        {
            debug::log(debug::level::inf,"-Successfully started %ud CPU cores",aprunning);
            printf("-Successfully started %ud CPU cores\n",aprunning);
        }
        else
        {
            debug::log(debug::level::err,"-Error starting CPU cores, only %ud/%ud started", aprunning,cpu::get_count()-1);
            printf("-Error starting CPU cores, only %ud/%ud started\n",aprunning,cpu::get_count()-1);
        }
    }
    uint8_t get_current_id()
    {
        auto lapic_id = apic::get_bspid();
        for(auto& x:cpu_array)
        {
            if(x.is_present)
            {
                if(x.lapic_id==lapic_id)
                    return x.processor_id;
            }
        }
        return 0xff;
    }
};