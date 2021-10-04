#include "include/memory.h"

namespace memory
{
    void memcpy(const void *dst, const void *src, size_t size)
    {
        for (uint64_t i = 0; i < size; i++)
        {
            ((uint8_t *)dst)[i] = ((uint8_t *)src)[i];
        }
    }
    void memset(void *addr, uint8_t fill, size_t size)
    {
        for (uint64_t i = 0; i < size; i++)
        {
            ((uint8_t *)addr)[i] = fill;
        }
    }
    bool memcmp(const void* dst, const void* src, size_t size)
    {
        for (uint64_t i = 0; i < size; i++)
        {
            if(((uint8_t *)dst)[i] != ((uint8_t *)src)[i])
                return false;
        }
        return true;
    }
    void *align(void *address, uint64_t align)
    {
        uint64_t _addr = (uint64_t)address;
        _addr = (_addr % align == 0 ? _addr : ((_addr + align - 1) / align) * align);
        return (void *)_addr;
    }
    void *normalize(void *address)
    {
        uint64_t _addr = (uint64_t)address;
        bool is_higher_half = _addr & 0xffff800000000000;
        return (void *)(is_higher_half ? (_addr | 0xffff800000000000) : _addr);
    }
    bool is_normalized(void *address)
    {
        return address == normalize(address);
    }
};