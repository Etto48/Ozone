#include "include/acpi.h"

namespace acpi
{
    RSDP_descriptor_2 *rsdp = nullptr;
    FADT *fadt = nullptr;
    MADT *madt = nullptr;
    bool rsdp_is_valid = false;

    template <typename T>
    bool checksum(const T &d)
    {
        uint8_t *p = (uint8_t *)&d;
        uint8_t sum = 0;
        for (uint64_t i = 0; i < sizeof(T); i++)
        {
            sum += p[i];
        }
        return sum == 0;
    }

    void init()
    {
        for (uint64_t i = 0x000E0000; i <= 0x000FFFFF - 8; i += 16)
        {
            auto str = (char *)i;
            if (memory::memcmp(str, "RSD PTR ", 8))
            {
                rsdp = (RSDP_descriptor_2 *)i;
                break;
            }
        }
        debug::log(debug::level::inf, "RSDP found at %p", rsdp);
        if (rsdp->first_part.revision == 0) //version 1
            debug::log(debug::level::inf, "ACPI version detected: 1.0");
        else //version >= 2
            debug::log(debug::level::inf, "ACPI version detected: 2.0 or higher");

        if (rsdp->first_part.revision == 0)
        {
            if (checksum(rsdp->first_part))
            {
                debug::log(debug::level::inf, "RSDP 1.0 Checksum validated");
                rsdp_is_valid = true;
            }
            else
                debug::log(debug::level::wrn, "RSDP 1.0 Checksum failed");
        }
        else if (rsdp->first_part.revision == 2) //checksum second part
        {
            if (checksum(*rsdp))
            {
                debug::log(debug::level::inf, "RSDP 2.0 Checksum validated");
                rsdp_is_valid = true;
            }
            else
                debug::log(debug::level::wrn, "RSDP 2.0 Checksum failed");
        }

        if (rsdp->first_part.revision == 0)
        { //we must use the RSDT
            auto rsdt = (ACPISDT_header *)(uint64_t)rsdp->first_part.rsdt_address;
            uint64_t entries = (rsdt->length - sizeof(ACPISDT_header)) / 4;
            auto sdt_pointer_array = (uint32_t *)((uint64_t)rsdt + sizeof(ACPISDT_header));
            for (uint64_t i = 0; i < entries; i++)
            {
                if (memory::memcmp(((ACPISDT_header *)(uint64_t)sdt_pointer_array[i])->signature, "FACP", 4))
                {
                    debug::log(debug::level::inf, "FACP found");
                    printf("FACP found\n");
                    fadt = (FADT *)(uint64_t)sdt_pointer_array[i];
                }
                else if (memory::memcmp(((ACPISDT_header *)(uint64_t)sdt_pointer_array[i])->signature, "APIC", 4))
                {
                    debug::log(debug::level::inf, "MADT found");
                    printf("MADT found\n");
                    madt = (MADT *)(uint64_t)sdt_pointer_array[i];
                }
                if(madt&&fadt)
                    break;
            }
        }
        else if (rsdp->first_part.revision == 2)
        {
            auto xsdt = (ACPISDT_header *)rsdp->xsdt_address;
            uint64_t entries = (xsdt->length - sizeof(ACPISDT_header)) / 8;
            auto sdt_pointer_array = (uint64_t *)((uint64_t)xsdt + sizeof(ACPISDT_header));
            for (uint64_t i = 0; i < entries; i++)
            {
                if (memory::memcmp(((ACPISDT_header *)sdt_pointer_array[i])->signature, "FACP", 4))
                {
                    debug::log(debug::level::inf, "FACP found");
                    printf("FACP found\n");
                    fadt = (FADT *)sdt_pointer_array[i];
                }
                else if (memory::memcmp(((ACPISDT_header *)sdt_pointer_array[i])->signature, "APIC", 4))
                {
                    debug::log(debug::level::inf, "MADT found");
                    printf("MADT found\n");
                    madt = (MADT *)sdt_pointer_array[i];
                }
            }
        }
    }

    void init_apic()
    {
        for(auto madt_entries_address = ((uint64_t)acpi::madt+0x2c);madt_entries_address<(uint64_t)acpi::madt+acpi::madt->h.length;)
        {
            size_t size = 0;
            switch(((acpi::MADT_entry_header*)madt_entries_address)->type)
            {
            case 0:
                debug::log(debug::level::inf,"LAPIC found");
                printf("LAPIC found, ID: %uld CPU_ID: %uld\n",(uint64_t)((acpi::local_APIC*)madt_entries_address)->APIC_id,(uint64_t)((acpi::local_APIC*)madt_entries_address)->processor_id);
                size = sizeof(acpi::local_APIC);
                {
                    auto lapic_address = ((acpi::local_APIC*)madt_entries_address);
                    cpu::cpu_array[lapic_address->processor_id].processor_id = lapic_address->processor_id;
                    cpu::cpu_array[lapic_address->processor_id].is_present = lapic_address->flags & 0b1 | lapic_address->flags & 0b10;
                    cpu::cpu_array[lapic_address->processor_id].lapic_id = lapic_address->APIC_id;
                }
                break;
            case 1:
                debug::log(debug::level::inf,"IOAPIC found");
                printf("IOAPIC found\n");
                size = sizeof(acpi::io_APIC);
                {
                    auto ioapic_address = ((acpi::io_APIC*)madt_entries_address);
                    //apic::to_ioapic((void*)(uint64_t)(ioapic_address->io_APIC_address));
                }
                break;
            case 2:
                debug::log(debug::level::inf,"IOAPIC ISO found");
                printf("IOAPIC ISO found\n");
                size = sizeof(acpi::io_APIC_interrupt_source_override);
                break;
            case 3:
                debug::log(debug::level::inf,"IOAPIC NMI IS found");
                printf("IOAPIC NMI IS found\n");
                size = sizeof(acpi::io_APIC_nmi_source);
                break;
            case 4:
                debug::log(debug::level::inf,"LAPIC NMI found");
                printf("LAPIC NMI found\n");
                size = sizeof(acpi::local_APIC_nmi_source);
                break;
            case 5:
                debug::log(debug::level::inf,"LAPIC AO found");
                printf("LAPIC AO found\n");
                size = sizeof(acpi::local_APIC_address_override);
                break;
            case 9:
                debug::log(debug::level::inf,"Lx2APIC found");
                printf("Lx2APIC found\n");
                size = sizeof(acpi::local_x2APIC);
                break;
            default:    
                debug::log(debug::level::wrn,"Unknown MADT entry type %ud",((acpi::MADT_entry_header*)madt_entries_address)->type);
                printf("Unknown MADT entry type %ud\n",((acpi::MADT_entry_header*)madt_entries_address)->type);
                goto exit;
                break;
            }
            if(((acpi::MADT_entry_header*)madt_entries_address)->length==0)
                break;
            madt_entries_address += ((acpi::MADT_entry_header*)madt_entries_address)->length;
        }
        debug::log(debug::level::inf,"%uld CPU cores found",cpu::get_count());
        printf("%uld CPU cores found\n",cpu::get_count());
    exit:
        return;
    }
};