#include "include/kernel_process.h"

namespace kernel
{

    volatile bool startup_done = false;
    void startup_loading_anim_sigterm()
    {  
        video::draw_image(loading_animation[24], {32, 32}, video::get_screen_size() / 2 - video::v2i{32, 32} / 2 + video::v2i{0,256/2});
        ozone::user::exit(0);
    }
    void startup_loading_anim()
    {
        ozone::user::set_signal_handler(ozone::SIGTERM,startup_loading_anim_sigterm);
        uint8_t i = 0;
        while (true)
        {
            video::draw_image(loading_animation[i++], {32, 32}, video::get_screen_size() / 2 - video::v2i{32, 32} / 2 + video::v2i{0,256/2});
            i%=24;
            ozone::user::sleep(36);
        }
    }
    void fork_modules()
    {
        multitasking::cli();
        if (boot_info::mbi.flags & MULTIBOOT_INFO_MODS && boot_info::mbi.mods_count >= 1)
        {
            debug::log(debug::level::inf,"%ud module(s) found", boot_info::mbi.mods_count);
            for (uint64_t mn = 0; mn < boot_info::mbi.mods_count; mn++)
            {
                auto &modr = ((multiboot_module_t *)(uint64_t)boot_info::mbi.mods_addr)[mn];
                auto ptrie = paging::create_paging_trie();
                auto level = memory::memcmp((void*)(uint64_t)modr.cmdline,"shell",4) ? interrupt::privilege_level_t::user : interrupt::privilege_level_t::system;
                multitasking::mapping_history_t* mh;
                auto entry = modules::load_module(&modr, ptrie, level, mh);
                if (entry == nullptr)
                    debug::log(debug::level::wrn,"  \e[31mModule %uld failed to load\e[0m\n", mn);
                else
                {
                    
                    auto pid = multitasking::create_process((void *)entry, ptrie, level, 0, mh);
                    debug::log(debug::level::wrn,"  Module %s(%uld) loaded as %s process %uld\n", (uint64_t)modr.cmdline, mn,
                        level==interrupt::privilege_level_t::system?"system":"user", pid);
                }
            }
        }
        else
        {
            debug::log(debug::level::wrn,"\e[31mcNo modules found\e[0m\n");
        }
        multitasking::sti();
    }
    void startup()
    {
        video::draw_image(ozone_logo[0], {256, 256}, video::get_screen_size() / 2 - video::v2i{256, 256} / 2);
        auto startup_loading_anim_id = ozone::user::fork(startup_loading_anim);
        //startup operations    
        kb::init();
        acpi::init_apic(); 
        cpu::start_cores(); 
        fork_modules();


        ozone::user::sleep(2000);
        //end startup operations
        ozone::user::signal(ozone::SIGTERM,startup_loading_anim_id);
        ozone::user::join(startup_loading_anim_id);
        printf("\e[%uld;%uldHWelcome to OZONE3 - 64", (video::get_screen_size() / 8 / 2).x - 11, (video::get_screen_size() / 8 / 2).y + 10);
        ozone::user::sleep(1000);
        
        for(uint64_t i = 0;i<8;i++)
        {
            video::draw_image(ozone_logo[i], {256, 256}, video::get_screen_size() / 2 - video::v2i{256, 256} / 2);
            ozone::user::sleep(36);
        }
        printf("\e[2J\e[H");
        stdout::enable();
        stdin::enable();
        startup_done = true;
    }
    void init()
    {
        //printf("\e[36mEntered multitasking mode\e[0m\n");
        apic::init();
        //printf("\e[32mApic initialized\e[0m\n");
        clock::init();
        //printf("\e[32mClock initialized\e[0m\n");
        //printf("\e[2J");
        stdout::init();
        stdin::init();
        ozone::user::fork(startup);
        while (!startup_done)
            ;
        //ozone::user::fork(status);
        //ozone::user::fork(test_ret);
        while (true)
            asm volatile("hlt");
    }

};