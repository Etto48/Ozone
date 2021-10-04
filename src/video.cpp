#include "include/video.h"


namespace video
{

    //ozone::semid_t video_handle = ozone::INVALID_SEMAPHORE;

    uint8_t red(color_t c)
    {
        //0xaarrggbb
        return (c & 0x00ff0000) >> 16;
    }
    uint8_t green(color_t c)
    {
        return (c & 0x0000ff00) >> 8;
    }
    uint8_t blue(color_t c)
    {
        return (c & 0x000000ff);
    }
    uint8_t alpha(color_t c)
    {
        return (c & 0xff000000) >> 24;
    }

    color_t multiply(color_t c, uint8_t a)
    {
        uint16_t r, g, b;
        r = red(c);
        g = green(c);
        b = blue(c);
        if (a != 0)
        {
            r /= (0x00ff / (uint16_t)a);
            g /= (0x00ff / (uint16_t)a);
            b /= (0x00ff / (uint16_t)a);
        }
        else
        {
            r = g = b = 0;
        }
        return rgb(uint8_t(r), uint8_t(g), uint8_t(b));
    }
    color_t multiply_colors(color_t c1, color_t c2)
    {
        uint16_t r1, g1, b1, a1, r2, g2, b2, a2;
        r1 = red(c1);
        g1 = green(c1);
        b1 = blue(c1);
        a1 = alpha(c1);
        r2 = red(c2);
        g2 = green(c2);
        b2 = blue(c2);
        a2 = alpha(c2);
        r1 /= (0x00ff / (uint16_t)r2);
        g1 /= (0x00ff / (uint16_t)g2);
        b1 /= (0x00ff / (uint16_t)b2);
        a1 /= (0x00ff / (uint16_t)a2);
        return rgba(uint8_t(r1), uint8_t(g1), uint8_t(b1), uint8_t(a1));
    }
    color_t add(color_t base, color_t cover)
    {
        uint8_t a = alpha(cover);
        color_t c1 = multiply(base, 0xff - a);
        color_t c2 = multiply(cover, a);
        uint8_t r = red(c1) + red(c2);
        uint8_t g = green(c1) + green(c2);
        uint8_t b = blue(c1) + blue(c2);
        return rgb(r, g, b);
    }

    v2i::v2i(uint64_t x, uint64_t y) : x(x), y(y) {}
    v2i v2i::operator+(v2i b)
    {
        return {x + b.x, y + b.y};
    }
    v2i v2i::operator-(v2i b)
    {
        return {x - b.x, y - b.y};
    }
    v2i v2i::operator*(v2i b)
    {
        return {x * b.x, y * b.y};
    }
    v2i v2i::operator/(v2i b)
    {
        return {x / b.x, y / b.y};
    }

    v2i v2i::operator*(uint64_t l)
    {
        return {x * l, y * l};
    }
    v2i v2i::operator/(uint64_t l)
    {
        return {x / l, y / l};
    }

    color_t *framebuffer = nullptr;
    uint64_t width = 0, height = 0;

    color_t buffers[MAX_BUFFER_COUNT][1024*768];

    color_t get_pixel(v2i position, uint64_t buffer_id)
    {
        if (buffer_id < MAX_BUFFER_COUNT && buffers[buffer_id])
        {
            return buffers[buffer_id][position.x + position.y * width];
        }
        return 0;
    }
    void set_pixel(v2i position, color_t color, uint64_t buffer_id)
    {
        if (buffer_id < MAX_BUFFER_COUNT)
        {
            buffers[buffer_id][position.x + position.y * width] = color;
        }
    }

    v2i get_screen_size()
    {
        return {width, height};
    }

    void init(color_t *framebuffer_address, uint64_t fb_width, uint64_t fb_height)
    {
        framebuffer = framebuffer_address;
        width = fb_width;
        height = fb_height;
        for(uint64_t i =0;i<MAX_BUFFER_COUNT;i++)
            clear(rgba(0,0,0,0),i);
        //buffers[BASE_BUFFER] = (color_t *)system_heap.malloc(sizeof(color_t) * width * height);
        //buffers[WINDOW_BUFFER] = (color_t *)system_heap.malloc(sizeof(color_t) * width * height);
        //buffers[CURSOR_BUFFER] = (color_t *)system_heap.malloc(sizeof(color_t) * width * height);

        clear(rgb(0, 0, 0));
        //video_handle = multitasking::create_semaphore(1);
    }

    void draw_image(color_t *image, v2i size, v2i position)
    {
        for (uint64_t y = 0; y < size.y; y++)
        {
            for (uint64_t x = 0; x < size.x; x++)
            {
                auto c = image[x + y * size.x];
                if (alpha(c) == 0xff)
                    pixel({x + position.x, y + position.y}) = c;
                else if (alpha(c) != 0)
                    pixel({x + position.x, y + position.y}) = multiply(c, alpha(c));
            }
        }
    }

    void clear(color_t color)
    {
        for (uint64_t y = 0; y < height; y++)
        {
            for (uint64_t x = 0; x < width; x++)
            {
                pixel({x, y}) = color;
            }
        }
    }
    void clear(color_t color,uint64_t buffer_id)
    {
        for (uint64_t y = 0; y < height; y++)
        {
            for (uint64_t x = 0; x < width; x++)
            {
                set_pixel({x,y},color,buffer_id);
            }
        }
    }

    color_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        //0xaarrggbb
        return a << 24 | r << 16 | g << 8 | b;
    }
    color_t rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        return rgba(r, g, b, 0xff);
    }

    void update_framebuffer()
    {
        for (uint64_t y = 0; y < height; y++)
        {
            for (uint64_t x = 0; x < width; x++)
            {
                bool drawn = false;
                for (int64_t b = MAX_BUFFER_COUNT; b > 0; b--)
                {
                    auto col = get_pixel({x,y},b-1);
                    if (alpha(col))
                    {
                        drawn = true;
                        pixel({x,y})=col;
                        break;
                    }
                }
                if(!drawn)
                {
                    pixel({x,y})=0;
                }
            }
        }
    }
};