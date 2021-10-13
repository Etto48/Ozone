#include "include/boot.h"



extern "C" void kmain()
{
    //clear(0x07);
    debug::init();
    debug::log(debug::level::inf, "---- OZONE for AMD64 ----");
    //printf("\e[47;30mWelcome to OZONE3 for AMD64!\n\e[0m");
    paging::init_frame_mapping(&boot_info::mbi);
    //printf("\e[32mFrame mapping initialized\e[0m\n");
    //printf("  kernel frames: %uld\n", paging::kernel_frames);
    //printf("  secondary frames: %uld\n", paging::secondary_frames);
    //printf("  available memory: %uld B\n", paging::free_frames * 0x1000);
    paging::extend_identity_mapping(&boot_info::mbi);
    //printf("\e[32mIdentity mapping extended\e[0m\n");
    //printf("  available memory: %uld B\n", paging::free_frames * 0x1000);
    if(boot_info::mbi.flags & MULTIBOOT_INFO_VBE_INFO)
    {

        video::init((video::color_t*)(uint64_t)boot_info::mbi.framebuffer_addr,boot_info::mbi.framebuffer_width,boot_info::mbi.framebuffer_height);
        printing::init();
        printf("\e[37;40m\e[2J\e[H");
        video::draw_image(ozone_logo[0],{256,256},video::get_screen_size()/2-video::v2i{256,256}/2);
        //printf("\e[35mFramebuffer found at 0x%p, %udx%ud colors:%ud\e[0m\n",boot_info::mbi.framebuffer_addr,boot_info::mbi.framebuffer_width,boot_info::mbi.framebuffer_height,boot_info::mbi.framebuffer_palette_num_colors);
    }
    else
    {
        video::init((video::color_t*)(uint64_t)boot_info::mbi.framebuffer_addr,boot_info::mbi.framebuffer_width,boot_info::mbi.framebuffer_height);
        printing::init();
        printf("\e[37;40m\e[2J\e[H");
        multitasking::abort("No framebuffer found");
        //printf("\e[31mFramebuffer not found\e[0m\n");
    }
    /*if(boot_info::mbi.flags & MULTIBOOT_INFO_MEM_MAP)
    {
        printf("\e[35mMemory map found\e[0m\n");
    }
    else
    {
        printf("\e[31mMemory map not found\e[0m\n");
    }
    printf("\e[32mAvailable RAM:%uldMiB\e[0m\n",paging::system_memory/(1024*1024));
    */
    acpi::init();
    /*printf("\e[32mACPI initialized\e[0m\n");
    printf("  ACPI version %s\n",acpi::rsdp->first_part.revision==0?"1.0":"2.0 or higher");
    printf("  OEM: %c%c%c%c%c%c\n",acpi::rsdp->first_part.OEMID[0],acpi::rsdp->first_part.OEMID[1],acpi::rsdp->first_part.OEMID[2],acpi::rsdp->first_part.OEMID[3],acpi::rsdp->first_part.OEMID[4],acpi::rsdp->first_part.OEMID[5]);
    if(acpi::rsdp_is_valid)
    {
        printf("  RSDP checksum validated\n");
    }*/
    
    interrupt::init_interrupts();
    //printf("\e[32mInterrupts initialized\e[0m\n");
    multitasking::init_process_array();
    //printf("\e[32mProcess array initialized\e[0m\n");
    acpi::init_apic();
    cpu::init();
    multitasking::create_process((void *)kernel::init, &paging::identity_l4_table, interrupt::privilege_level_t::system, multitasking::MAX_PROCESS_NUMBER,nullptr);
    //fork_modules();
    ozone::user::sys_call_n(0); //create an interrupt to switch to multitasking mode
}
