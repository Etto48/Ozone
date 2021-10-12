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
        if(fadt)
        {
            debug::log(debug::level::inf,"System Control Interrupt: %ud",fadt->SCI_Interrupt);
            char devt_strings[][20]={"Unspecified","Desktop","Mobile","Workstation","Enterprise Sever","SOHO Server","Aplliance PC","Performance Server","Reserved"};
            debug::log(debug::level::inf,"Device Type: %s",devt_strings[fadt->PreferredPowerManagementProfile]);
        }
    }

    void init_apic()
    {
        debug::log(debug::level::inf,"APIC");
        printf("APIC\n");
        for(auto madt_entries_address = ((uint64_t)acpi::madt+0x2c);madt_entries_address<(uint64_t)acpi::madt+acpi::madt->h.length;)
        {
            size_t size = 0;
            switch(((acpi::MADT_entry_header*)madt_entries_address)->type)
            {
            case 0:
                debug::log(debug::level::inf,"-LAPIC found, ID: %uld, CPU_ID: %uld",(uint64_t)((acpi::local_APIC*)madt_entries_address)->APIC_id,(uint64_t)((acpi::local_APIC*)madt_entries_address)->processor_id);
                printf("-LAPIC found, ID: %uld, CPU_ID: %uld\n",(uint64_t)((acpi::local_APIC*)madt_entries_address)->APIC_id,(uint64_t)((acpi::local_APIC*)madt_entries_address)->processor_id);
                size = sizeof(acpi::local_APIC);
                {
                    auto lapic_address = ((acpi::local_APIC*)madt_entries_address);
                    cpu::cpu_array[lapic_address->processor_id].processor_id = lapic_address->processor_id;
                    cpu::cpu_array[lapic_address->processor_id].is_present = lapic_address->flags & 0b1 | lapic_address->flags & 0b10;
                    cpu::cpu_array[lapic_address->processor_id].lapic_id = lapic_address->APIC_id;
                }
                break;
            case 1:
                debug::log(debug::level::inf,"-IOAPIC found");
                printf("-IOAPIC found\n");
                size = sizeof(acpi::io_APIC);
                {
                    auto ioapic_address = ((acpi::io_APIC*)madt_entries_address);
    
                    apic::to_ioapic((void*)(uint64_t)(ioapic_address->io_APIC_address),(void*)(uint64_t)(madt->local_apic_address));
                    //debug::log(debug::level::inf,"READ LAPIC addr %p");
                    //printf("READ APIC ver %x\n",apic::in(1));
                }
                break;
            case 2:
                size = sizeof(acpi::io_APIC_interrupt_source_override);
                {
                    auto ioapiciso_address = ((acpi::io_APIC_interrupt_source_override*)madt_entries_address);
                    debug::log(debug::level::inf,"-IOAPIC ISO found: %uld -> %uld",(uint64_t)ioapiciso_address->IRQ_source,(uint64_t)ioapiciso_address->global_system_interrupt);
                    printf("-IOAPIC ISO found: %uld -> %uld\n",(uint64_t)ioapiciso_address->IRQ_source,(uint64_t)ioapiciso_address->global_system_interrupt);
                    apic::set_VECT(ioapiciso_address->global_system_interrupt,interrupt::ISR_SIZE+ioapiciso_address->IRQ_source);
                    if(ioapiciso_address->flags & 2)
                    {
                        apic::set_IPOL(ioapiciso_address->global_system_interrupt,true);
                        debug::log(debug::level::inf,"--Interrupt %ud set to active low",ioapiciso_address->global_system_interrupt);
                        printf("--Interrupt %ud set to active low",ioapiciso_address->global_system_interrupt);
                    }
                    if(ioapiciso_address->flags & 8)
                    {
                        apic::set_TRGM(ioapiciso_address->global_system_interrupt,true);
                        debug::log(debug::level::inf,"--Interrupt %ud set to level triggered",ioapiciso_address->global_system_interrupt);
                        printf("--Interrupt %ud set to level triggered",ioapiciso_address->global_system_interrupt);
                    }
                }
                break;
            case 3:
                size = sizeof(acpi::io_APIC_nmi_source);
                {
                    auto ioapicnmis_address = ((acpi::io_APIC_nmi_source*)madt_entries_address);
                    debug::log(debug::level::inf,"-IOAPIC NMIS found, NMI_source: %ud, GSI: %ud",ioapicnmis_address->NMI_source,ioapicnmis_address->global_system_interrupt);
                    printf("-IOAPIC NMIS found, NMI_source: %ud, GSI: %ud\n",ioapicnmis_address->NMI_source,ioapicnmis_address->global_system_interrupt);
                    apic::set_DELM(ioapicnmis_address->global_system_interrupt,0b100);
                    if(ioapicnmis_address->flags & 2)
                    {
                        apic::set_IPOL(ioapicnmis_address->global_system_interrupt,true);
                        debug::log(debug::level::inf,"--Interrupt %ud set to active low",ioapicnmis_address->global_system_interrupt);
                        printf("--Interrupt %ud set to active low",ioapicnmis_address->global_system_interrupt);
                    }
                    if(ioapicnmis_address->flags & 8)
                    {
                        apic::set_TRGM(ioapicnmis_address->global_system_interrupt,true);
                        debug::log(debug::level::inf,"--Interrupt %ud set to level triggered",ioapicnmis_address->global_system_interrupt);
                        printf("--Interrupt %ud set to level triggered",ioapicnmis_address->global_system_interrupt);
                    }

                }
                break;
            case 4:
                debug::log(debug::level::inf,"-LAPIC NMI found");
                printf("-LAPIC NMI found\n");
                size = sizeof(acpi::local_APIC_nmi_source);
                break;
            case 5:
                debug::log(debug::level::inf,"-LAPIC AO found");
                printf("-LAPIC AO found\n");
                size = sizeof(acpi::local_APIC_address_override);
                {
                    auto lapicao_address = ((acpi::local_APIC_address_override*)madt_entries_address);
                    apic::set_lapic_base((void*)lapicao_address->local_APIC_phisical_address);
                }
                break;
            case 9:
                debug::log(debug::level::inf,"-Lx2APIC found");
                printf("-Lx2APIC found\n");
                size = sizeof(acpi::local_x2APIC);
                break;
            default:    
                debug::log(debug::level::wrn,"-Unknown MADT entry type %ud",((acpi::MADT_entry_header*)madt_entries_address)->type);
                printf("-Unknown MADT entry type %ud\n",((acpi::MADT_entry_header*)madt_entries_address)->type);
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