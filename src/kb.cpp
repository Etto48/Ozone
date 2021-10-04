#include "include/kb.h"
#include "include/en_us.h"

namespace kb
{

    constexpr uint16_t KEYBOARD_DATA_PORT = 0x60;
    constexpr uint16_t KEYBOARD_STATUS_PORT = 0x64;

    volatile bool is_caps_lock_active = false;
    volatile bool is_num_lock_active = false;
    volatile bool is_scroll_lock_active = false;

    volatile bool is_key_pressed[128] = {};

    #define R_SHIFT 42
    #define L_SHIFT 54
    #define CAPS_LOCK 58
    #define NUM_LOCK 69
    #define SCROLL_LOCK 70
    #define LEFT_ARROW 75
    #define RIGHT_ARROW 77
    #define HOME 71
    #define END 79
    #define CTRL 29
    #define ALT 56
    #define BACKSPACE 14
    #define DELETE 83

    void init()
    {
        interrupt::irq_callbacks[1] = callback;
    }
    void kbd_ack(void)
    {
        while (io::inb(KEYBOARD_DATA_PORT) != 0xfa);
    }

    bool shift_is_pressed()
    {
        return is_caps_lock_active || is_key_pressed[L_SHIFT] || is_key_pressed[R_SHIFT];
    }
    void set_kb_leds(bool caps_lock, bool num_lock, bool scroll_lock)
    {
        uint8_t led_state = (caps_lock ? 0b00000100 : 0) | (num_lock ? 0b00000010 : 0) | (scroll_lock ? 0b00000001 : 0);

        io::outb(KEYBOARD_DATA_PORT, 0xed);
        kbd_ack();
        io::outb(KEYBOARD_DATA_PORT, led_state);
    }

    void callback(interrupt::context_t *context)
    {
        uint8_t scancode = io::inb(KEYBOARD_DATA_PORT);
        uint8_t state = io::inb(KEYBOARD_STATUS_PORT);
        if (state)
        {
            bool led_changed = false;
            if (scancode & 0x80) //key up
            {
                scancode = scancode & ~0x80;
                is_key_pressed[scancode] = false;
            }
            else //key down/hold
            {
                if (!is_key_pressed[scancode])
                { //key down
                    switch (scancode)
                    {
                    case CAPS_LOCK:
                        is_caps_lock_active = !is_caps_lock_active;
                        led_changed = true;
                        break;
                    case NUM_LOCK:
                        is_num_lock_active = !is_num_lock_active;
                        led_changed = true;
                        break;
                    case SCROLL_LOCK:
                        is_scroll_lock_active = !is_scroll_lock_active;
                        led_changed = true;
                        break;
                    default:
                        break;
                    }
                }
                //hold

                //check if need to return char
                if (kb_us[scancode] && stdin::is_waiting)
                {
                    char c = shift_is_pressed()?kb_us_caps[scancode]:kb_us[scancode];
                    stdin::return_char(c);
                    //debug::log(debug::level::inf,"Key %c pressed",c);
                }
                is_key_pressed[scancode] = true;
            }
            if (led_changed)
                set_kb_leds(is_caps_lock_active, is_num_lock_active, is_scroll_lock_active);
        }
    }
};