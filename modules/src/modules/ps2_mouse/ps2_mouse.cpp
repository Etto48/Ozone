#include <io.h>
#include <ozone.h>

#define max(A,B) ((A)>(B)?(A):(B))
#define min(A,B) ((A)<(B)?(A):(B))
#define abs(x) (max((x),-(x)))

bool mouse_btns[] = {0, 0, 0}; //middle,right,left
int32_t mouse_delta[] = {0, 0};  //x,y
//ozone::semid_t event_sync = ozone::INVALID_SEMAPHORE;

uint64_t wait_for_event() //returns true if is button related
{
    //ozone::user::acquire_semaphore(event_sync);
    bool old_mouse_btns[] = {mouse_btns[0], mouse_btns[1], mouse_btns[2]};
    while (0 == mouse_delta[0] && 0 == mouse_delta[1] && old_mouse_btns[0] == mouse_btns[0] && old_mouse_btns[1] == mouse_btns[1] && old_mouse_btns[2] == mouse_btns[2])
        ;

    return mouse_btns[0] || mouse_btns[1] || mouse_btns[2];
}
uint64_t get_mouse_buttons()
{
    return (1) * mouse_btns[0] | (1 << 1) * mouse_btns[1] | (1 << 2) * mouse_btns[2];
}
uint64_t get_mouse_delta()
{
    int32_t ret[2] = {mouse_delta[0], mouse_delta[1]};
    mouse_delta[0]=0;
    mouse_delta[1]=0;
    return uint64_t(ret[0]) | (uint64_t(ret[1]) << 32);
}

void mouse_wait(unsigned char type)
{
    unsigned int _time_out = 100000;
    if (type == 0)
    {
        while (_time_out--) //Data
        {
            if ((io::inb(0x64) & 1) == 1)
            {
                return;
            }
        }
        return;
    }
    else
    {
        while (_time_out--) //Signal
        {
            if ((io::inb(0x64) & 2) == 0)
            {
                return;
            }
        }
        return;
    }
}

uint8_t mouse_read()
{
    //Get response from mouse
    while(!(io::inb(0x64) & 1))
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
    io::outb(0xD4, 0x64);                    // tell the controller to address the mouse
    io::outb(a_write, 0x60);                    // write the mouse command code to the controller's data port
    mouse_read();                    // read back acknowledge. This should be 0xFA
}

void set_sample_rate(uint16_t sample_rate)
{
    mouse_write(0xF3);//set_sample_rate
    mouse_write(sample_rate);//sample rate
}
uint8_t identify()
{
    mouse_write(0xF2);
    return mouse_read();
}
void reset()
{
    mouse_write(0xFF);
    mouse_read();//BAT
    mouse_read();//ID
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
int main()
{
    ozone::user::exit(0);
    //event_sync = ozone::user::create_semaphore(0);
    ozone::system::set_driver(12, ozone::user::get_id());
    
    io::outb(0x64,0xA8);
    io::outb(0x64,0x20);
    auto status = io::inb(0x60) | 2;
    io::outb(0x64,0x60);
    io::outb(0x60,status);
    io::outb(0x64,0xD4);
    io::outb(0x60,0xF4);
    io::inb(0x60);

    reset();
    if(scroll_mode())
        key5_mode();
    set_sample_rate(100);
    set_resolution(3);
    auto device_id = identify();
    uint8_t offset = 0;

    ozone::system::set_driver_function(12, 0, get_mouse_delta);
    ozone::system::set_driver_function(12, 1, get_mouse_buttons);
    ozone::system::set_driver_function(12, 2, wait_for_event);
    while (true)
    {
        ozone::system::wait_for_interrupt();
        if (io::inb(0x64) & 0x20)
        {
            uint8_t p[4];
            p[offset] = io::inb(0x60);

            offset = (offset + 1) % (device_id==0?3:4);
            
            //bool y_overflow=p[0]&0b10000000;
            //bool x_overflow=p[0]&0b1000000;
            if(offset==0)
            {
            bool middle_btn = p[0] & 0b100;
            bool right_btn = p[0] & 0b10;
            bool left_btn = p[0] & 1;

            auto state = p[0];
	        auto d = p[1];
	        auto rel_x = d - ((state << 4) & 0x100);
	        d = p[2];
	        auto rel_y = d - ((state << 3) & 0x100);

            mouse_btns[0] = middle_btn;
            mouse_btns[1] = right_btn;
            mouse_btns[2] = left_btn;
            mouse_delta[0] = rel_x;
            mouse_delta[1] = -rel_y;

            //ozone::user::release_semaphore(event_sync);
            }
        }
    }
}