#include "include/shell.h"

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    //0xaarrggbb
    return a << 24 | r << 16 | g << 8 | b;
}
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return rgba(r, g, b, 0xff);
}
uint64_t complete_color(uint32_t fg, uint32_t bg)
{
    return uint64_t(bg) << 32 | fg;
}