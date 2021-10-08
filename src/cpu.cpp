#include "include/cpu.h"

namespace cpu
{
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
};