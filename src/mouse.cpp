#include "include/mouse.h"

namespace mouse
{
    uint8_t mouse_read()
    {
        //Get response from mouse
        while (!(io::inb(0x64) & 1))
            asm volatile("pause"); // wait until we can read
        return io::inb(0x60);
    }

    void mouse_write(unsigned char a_write)
    {
        /*//Wait to be able to send a command
    mouse_wait(1);
    //Tell the mouse we are sending a command
    io::outb(0x64, 0xD4);
    //Wait for the final part
    mouse_wait(1);
    //Finally write
    io::outb(0x60, a_write);*/
        io::outb(0xD4, 0x64);    // tell the controller to address the mouse
        io::outb(a_write, 0x60); // write the mouse command code to the controller's data port
        mouse_read();            // read back acknowledge. This should be 0xFA
    }

    void set_sample_rate(uint16_t sample_rate)
    {
        mouse_write(0xF3);        //set_sample_rate
        mouse_write(sample_rate); //sample rate
    }
    uint8_t identify()
    {
        mouse_write(0xF2);
        return mouse_read();
    }
    void reset()
    {
        mouse_write(0xFF);
        mouse_read(); //BAT
        mouse_read(); //ID
        mouse_write(0xF4);
    }
    bool scroll_mode()
    {
        set_sample_rate(200);
        set_sample_rate(100);
        set_sample_rate(80);
        return identify() == 3;
    }
    bool key5_mode()
    {
        set_sample_rate(200);
        set_sample_rate(200);
        set_sample_rate(80);
        return identify() == 4;
    }
    void set_resolution(uint8_t res_id)
    {
        mouse_write(0xE8);
        mouse_write(res_id);
    }

    uint8_t device_id = 0;
    uint8_t offset = 0;

    void init()
    {
        interrupt::irq_callbacks[12] = callback;

        io::outb(0x64, 0xA8);
        io::outb(0x64, 0x20);
        auto status = io::inb(0x60) | 2;
        io::outb(0x64, 0x60);
        io::outb(0x60, status);
        io::outb(0x64, 0xD4);
        io::outb(0x60, 0xF4);
        io::inb(0x60);

        reset();
        if (scroll_mode())
            key5_mode();
        set_sample_rate(200);
        set_resolution(3);
        device_id = identify();
    }
    bool discard_next = false;

    bool mouse_btns[3] = {false, false, false};
    video::v2i mouse_pos{0, 0};
    video::v2i old_mouse_pos{0, 0};
    uint8_t p[4];

    int inline bytetosignedshort(uint8_t a, bool s)
    {
        return ((s) ? 0xFFFFFF00 : 0x00000000) | a;
    }

    void callback(interrupt::context_t *context)
    {
        if (io::inb(0x64) & 0x20)
        {
            if (discard_next)
            {
                discard_next = false;
                io::inb(0x60);
                return;
            }
            p[offset] = io::inb(0x60);

            offset = (offset + 1) % (device_id == 0 ? 3 : 4);
            if (!(p[0] & 0x08))
            {
                discard_next = true;
                offset=0;
                return;
            }

            if (offset == 0)
            {
                if (p[0] & 0x80 || p[0] & 0x40)//overflow
                    return;

                bool middle_btn = p[0] & 0b100;
                bool right_btn = p[0] & 0b10;
                bool left_btn = p[0] & 1;

                auto state = p[0];
                auto rel_x = bytetosignedshort(p[1],(p[0] & 0x10) >> 4);
                auto rel_y = bytetosignedshort(p[2],(p[0] & 0x20) >> 5);

                old_mouse_pos = mouse_pos;

                constexpr int DX_DIV = 1;
                constexpr int DY_DIV = 1;

                int new_x = mouse_pos.x +(rel_x/DX_DIV);
                int new_y = mouse_pos.y -(rel_y/DY_DIV);

                mouse_btns[0] = middle_btn;
                mouse_btns[1] = right_btn;
                mouse_btns[2] = left_btn;

                auto ss = video::get_screen_size();

                if(new_x<0)
    				mouse_pos.x=0;
			    else if(new_x>=ss.x)
    				mouse_pos.x=ss.x-1;
			    else
				    mouse_pos.x=new_x;

                if(new_y<0)
    				mouse_pos.y=0;
			    else if(new_y>=ss.y)
    				mouse_pos.y=ss.y-1;
			    else
				    mouse_pos.y=new_y;

                //debug::log(debug::level::inf,"MOUSE X:%d Y:%d",mouse_pos.x,mouse_pos.y);
            }
        }
    }
    video::v2i get_pos()
    {
        return mouse_pos;
    }
    bool left_button()
    {
        return mouse_btns[2];
    }
    bool right_button()
    {
        return mouse_btns[1];
    }
    bool middle_button()
    {
        return mouse_btns[0];
    }
};