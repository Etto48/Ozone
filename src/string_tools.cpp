#include "include/string_tools.h"

namespace string_tools
{
    char *itoa(long int value, char *result, int base, unsigned int min_len)
    {
        // check that the base if valid
        if (base < 2 || base > 36)
        {
            *result = '\0';
            return result;
        }

        char *ptr = result, *ptr1 = result, tmp_char;
        long int tmp_value;
        unsigned int n = 0;
        do
        {
            tmp_value = value;
            value /= base;
            *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
            n++;
        } while (value || n < min_len);

        // Apply negative sign
        if (tmp_value < 0)
            *ptr++ = '-';
        *ptr-- = '\0';
        while (ptr1 < ptr)
        {
            tmp_char = *ptr;
            *ptr-- = *ptr1;
            *ptr1++ = tmp_char;
        }
        return result;
    }

    char *utoa(unsigned long value, char *result, int base, unsigned int min_len)
    {
        // check that the base if valid
        if (base < 2 || base > 36)
        {
            *result = '\0';
            return result;
        }

        char *ptr = result, *ptr1 = result, tmp_char;
        unsigned long tmp_value;
        unsigned int n = 0;
        do
        {
            tmp_value = value;
            value /= base;
            *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value % base];
            n++;
        } while (value || n < min_len);

        *ptr-- = '\0';
        while (ptr1 < ptr)
        {
            tmp_char = *ptr;
            *ptr-- = *ptr1;
            *ptr1++ = tmp_char;
        }
        return result;
    }

    int atoi(char *str, int base)
    {
        int res = 0; // Initialize result
        uint8_t begin = 0;
        if (str[0] == '-' || str[0] == '+')
            begin = 1;
        // Iterate through all characters of input string and update result
        for (int i = begin; str[i] != '\0'; ++i)
            res = res * base + str[i] - '0';
        if (str[0] == '-')
            res = -res;
        // return result.
        return res;
    }

    static const char hex_map[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    void htostr(char *buf, unsigned long long l, int n)
    {
        int i;
        for (i = n - 1; i >= 0; --i)
        {
            buf[i] = hex_map[l % 16];
            l /= 16;
        }
    }
    void itostr(char *buf, unsigned int len, long l)
    {
        unsigned int i, div = 1000000000, v, w = 0;

        if (l == (-2147483647 - 1))
        {
            strncpy(buf, "-2147483648", 12);
            return;
        }
        else if (l < 0)
        {
            buf[0] = '-';
            l = -l;
            i = 1;
        }
        else if (l == 0)
        {
            buf[0] = '0';
            buf[1] = 0;
            return;
        }
        else
            i = 0;

        while (i < len - 1 && div != 0)
        {
            if ((v = l / div) || w)
            {
                buf[i++] = '0' + (char)v;
                w = 1;
            }

            l %= div;
            div /= 10;
        }

        buf[i] = 0;
    }

    size_t strlen(const char *s)
    {
        size_t l = 0;

        while (*s++)
            ++l;

        return l;
    }

    char *strncpy(char *dest, const char *src, uint32_t l)
    {
        size_t i;

        for (i = 0; i < l && src[i]; ++i)
            dest[i] = src[i];

        return dest;
    }

};