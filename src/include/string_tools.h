#pragma once
#include <stdint.h>
#include <stddef.h>

namespace string_tools
{
    int atoi(char* str, int base=10);
    char* utoa(unsigned long value, char *result, int base=10,unsigned int min_len = 0);
    char* itoa(long int value, char* result, int base=10,unsigned int min_len = 0);
    void htostr(char *buf, unsigned long long l, int n);
    void itostr(char *buf, unsigned int len, long l);
    size_t strlen(const char *s);
    char *strncpy(char *dest, const char *src, uint32_t l);
};