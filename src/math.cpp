#include "include/math.h"

double pow(double x, int64_t n)
{
    uint32_t iter = abs(n);
    double ret = 1;
    if(n<0)
    {
        for(uint32_t i=0;i<iter;i++)
        {
            ret/=x;
        }
    }
    else
    {
        for(uint32_t i=0;i<iter;i++)
        {
            ret*=x;
        }
    }
    return ret;
}