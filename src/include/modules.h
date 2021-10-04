#pragma once
#include <stdint.h>
#include "multiboot.h"
#include "elf64.h"
#include "printing.h"
#include "memory.h"
#include "paging.h"
#include "interrupt.h"

namespace modules
{
    //loads a module into memory and returns its entrypoint
    void(*load_module(multiboot_module_t* mod, paging::page_table_t* root_tab, interrupt::privilege_level_t privilege_level, multitasking::mapping_history_t*& mapping_history))();
};