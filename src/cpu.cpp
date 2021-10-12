#include "include/cpu.h"

namespace cpu
{
    uint8_t bspdone = 0;
    volatile uint8_t aprunning = 0;

    uint32_t sizeof_cpu_descriptor = sizeof(cpu_descriptor_t);
    uint32_t cpu_array_pointer = (uint32_t)(uint64_t)&cpu_array;

    uint32_t gdtr_offset = 3 + sizeof(tss_t) + sizeof(gdt_t);
    uint32_t stack_base_offset = 3 + sizeof(tss_t) + sizeof(gdt_t) + sizeof(gdtr_t) + 1 + sizeof(kernel_stack_t) - 1;

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
    uint64_t get_booted_count()
    {
        uint64_t ret = 0;
        for(auto& x: cpu_array)
        {
            if(x.is_booted)
                ret++;
        }
        return ret;
    }
    void start_cores()
    {   
        debug::log(debug::level::inf,"-Starting %ud more CPU cores",cpu::get_count()-1);
        printf("-Starting %ud more CPU cores\n",cpu::get_count()-1);

        
        multitasking::cli();
        aprunning = 0;
        bspdone = 0;
        //copy ap_trampoline to destination address
        memory::memcpy((void*)0x8000,(void*)&ap_trampoline,4096);
        auto bspid = apic::get_bspid();
        for(auto& x: cpu_array)
        {
            if(x.is_present && !x.is_booted)
            {
                if(x.lapic_id==bspid)
                {
                    x.is_booted = true;
                    continue;
                }
                
                apic::send_INIT(x.lapic_id);
                multitasking::sti();
                clock::mwait(10);
                multitasking::cli();
                for(uint8_t j=0;j<2;j++)
                {
                    apic::send_SIPI(x.lapic_id,0x000608);
                }
            }
        }
        bspdone = 1;
        
        multitasking::sti();

        clock::mwait(20);
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

        clock::mwait(20);
        auto booted_count = cpu::get_booted_count();
        if(cpu::get_count()==booted_count)
        {
            debug::log(debug::level::inf,"-Successfully brought %uld CPU cores to 64b mode",booted_count-1);
            printf("-Successfully brought %uld CPU cores to 64b mode\n",booted_count-1);
        }
        else
        {
            debug::log(debug::level::err,"-Error bringing CPU cores to 64b mode, only %uld/%uld successful cores", booted_count-1,cpu::get_count()-1);
            printf("-Error bringing CPU cores to 64b mode, only %uld/%uld successful cores\n",booted_count-1,cpu::get_count()-1);
            for(auto& x:cpu_array)
            {
                if(x.is_present && ! x.is_booted)
                {
                    printf("--Missing CPU core ID:%ud, LAPIC_ID:%ud\n",x.processor_id,x.lapic_id);
                }
            }
        }
        
    }
    uint8_t get_current_processor_id()
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

    void ap64main()
    {
        auto processor_id = get_current_processor_id();
        cpu_array[processor_id].is_booted = true;

        cpu_array[processor_id].tss.rsp0 = uint64_t(&(cpu_array[processor_id].kernel_stack)) + sizeof(kernel_stack_t) - 1;
        asm volatile("movw $0x20, %cx\n or $3, %cx\n ltr %cx\n");
        
        //initialize IDT

        //here goes the smp code
        while(1);
    }
};