#pragma once
#include "printing.h"
#include "interrupt.h"
#include "apic.h"
#include "acpi.h"
#include "cpu.h"
#include "clock.h"
#include <ozone.h>
#include "boot_info.h"
#include "modules.h"
#include "pci.h"
#include "video.h"
#include "logo.h"
#include "loading_animation.h"
#include "stdout.h"
#include "kb.h"

namespace kernel
{
    extern ozone::semid_t framebuffer_mutex;
    void init();
};