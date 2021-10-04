#pragma once
#include <stdint.h>
#include "heap.h"

namespace video
{

    typedef uint32_t color_t;

    enum buffer_ids
    {
        WINDOW_BUFFER = 0,
        CURSOR_BUFFER = 1,
    };

    struct v2i
    {
        uint64_t x,y;
        v2i(uint64_t x=0,uint64_t y=0);
        v2i operator +(v2i b);
        v2i operator -(v2i b);
        v2i operator *(v2i b);
        v2i operator /(v2i b);

        v2i operator *(uint64_t l);
        v2i operator /(uint64_t l);
    };

    uint8_t red(color_t c);
    uint8_t green(color_t c);
    uint8_t blue(color_t c);
    uint8_t alpha(color_t c);

    color_t multiply(color_t c, uint8_t a);
    color_t multiply_colors(color_t c1, color_t c2);
    color_t add(color_t base, color_t cover);

    v2i get_screen_size();

    void init(color_t* framebuffer_address,uint64_t width, uint64_t height);

    constexpr uint64_t MAX_BUFFER_COUNT = 2;

    //extern ozone::semid_t video_handle;

    extern color_t buffers[MAX_BUFFER_COUNT][1024*768];

    extern color_t* framebuffer;
    extern uint64_t width, height;

    color_t get_pixel(v2i position,uint64_t buffer_id);
    void set_pixel(v2i position, color_t color, uint64_t buffer_id);

    inline color_t &pixel(v2i position)
    {
        return framebuffer[position.x % width + (position.y % height) * width];
    }

    void draw_image(color_t* image, v2i size, v2i position);
    void clear(color_t color);
    void clear(color_t color,uint64_t buffer_id);

    color_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    color_t rgb(uint8_t r, uint8_t g, uint8_t b);

    void update_framebuffer();
    
};
