#pragma once
#include <stdint.h>
#include "memory.h"
#include "debug.h"
#include "cpu.h"

namespace acpi
{
    struct RSDP_descriptor
    {
        char signature[8];
        uint8_t checksum;
        char OEMID[6];
        uint8_t revision;
        uint32_t rsdt_address;
    } __attribute__((packed));

    struct RSDP_descriptor_2
    {
        RSDP_descriptor first_part;
        uint32_t length;
        uint64_t xsdt_address;
        uint8_t extended_checksum;
        uint8_t reserved[3];
    } __attribute__((packed));

    struct ACPISDT_header
    {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char OEMID[6];
        char OEMTableID[8];
        uint32_t OEM_revision;
        uint32_t creator_id;
        uint32_t creator_revision;
    };

    struct GenericAddressStructure
    {
        uint8_t AddressSpace;
        uint8_t BitWidth;
        uint8_t BitOffset;
        uint8_t AccessSize;
        uint64_t Address;
    };

    struct FADT
    {
        struct ACPISDT_header h;
        uint32_t FirmwareCtrl;
        uint32_t Dsdt;

        // field used in ACPI 1.0; no longer in use, for compatibility only
        uint8_t Reserved;

        uint8_t PreferredPowerManagementProfile;
        uint16_t SCI_Interrupt;
        uint32_t SMI_CommandPort;
        uint8_t AcpiEnable;
        uint8_t AcpiDisable;
        uint8_t S4BIOS_REQ;
        uint8_t PSTATE_Control;
        uint32_t PM1aEventBlock;
        uint32_t PM1bEventBlock;
        uint32_t PM1aControlBlock;
        uint32_t PM1bControlBlock;
        uint32_t PM2ControlBlock;
        uint32_t PMTimerBlock;
        uint32_t GPE0Block;
        uint32_t GPE1Block;
        uint8_t PM1EventLength;
        uint8_t PM1ControlLength;
        uint8_t PM2ControlLength;
        uint8_t PMTimerLength;
        uint8_t GPE0Length;
        uint8_t GPE1Length;
        uint8_t GPE1Base;
        uint8_t CStateControl;
        uint16_t WorstC2Latency;
        uint16_t WorstC3Latency;
        uint16_t FlushSize;
        uint16_t FlushStride;
        uint8_t DutyOffset;
        uint8_t DutyWidth;
        uint8_t DayAlarm;
        uint8_t MonthAlarm;
        uint8_t Century;

        // reserved in ACPI 1.0; used since ACPI 2.0+
        uint16_t BootArchitectureFlags;

        uint8_t Reserved2;
        uint32_t Flags;

        // 12 byte structure; see below for details
        GenericAddressStructure ResetReg;

        uint8_t ResetValue;
        uint8_t Reserved3[3];

        // 64bit pointers - Available on ACPI 2.0+
        uint64_t X_FirmwareControl;
        uint64_t X_Dsdt;

        GenericAddressStructure X_PM1aEventBlock;
        GenericAddressStructure X_PM1bEventBlock;
        GenericAddressStructure X_PM1aControlBlock;
        GenericAddressStructure X_PM1bControlBlock;
        GenericAddressStructure X_PM2ControlBlock;
        GenericAddressStructure X_PMTimerBlock;
        GenericAddressStructure X_GPE0Block;
        GenericAddressStructure X_GPE1Block;
    };

    struct MADT
    {
        ACPISDT_header h;
        uint32_t local_apic_address;
        uint32_t flags;
    }__attribute__((packed));

    struct MADT_entry_header
    {
        uint8_t type;
        uint8_t length;
    }__attribute__((packed));
    struct local_APIC //madt entry type 0
    {
        MADT_entry_header h;
        uint8_t processor_id;
        uint8_t APIC_id;
        uint32_t flags;
    }__attribute__((packed));
    struct io_APIC //madt entry type 1
    {
        MADT_entry_header h;
        uint8_t io_APIC_id;
        uint8_t reserved;
        uint32_t io_APIC_address;
        uint32_t global_system_interrupt_base;
    }__attribute__((packed));
    struct io_APIC_interrupt_source_override //madt entry type 2
    {
        MADT_entry_header h;
        uint8_t bus_source;
        uint8_t IRQ_source;
        uint32_t global_system_interrupt;
        uint16_t flags;
    }__attribute__((packed));
    struct io_APIC_nmi_source//3
    {
        MADT_entry_header h;
        uint8_t ACPI_processor_id;
        uint16_t flags;
        uint8_t LINT;
    }__attribute__((packed));
    struct local_APIC_nmi_source//4
    {
        MADT_entry_header h;
        uint8_t ACPI_processor_id;
        uint16_t flags;
        uint8_t LINT;
    }__attribute__((packed));
    struct local_APIC_address_override//5
    {
        MADT_entry_header h;
        uint16_t reserved;
        uint64_t local_APIC_phisical_address;
    }__attribute__((packed));
    struct local_x2APIC//9
    {
        MADT_entry_header h;
        uint16_t reserved;
        uint32_t processor_local_x2APIC_id;
        uint32_t flags;
        uint32_t ACPI_id;
    }__attribute__((packed));

    extern RSDP_descriptor_2 *rsdp;
    extern FADT *fadt;
    extern MADT *madt;
    extern bool rsdp_is_valid;

    void init();
    void init_apic();
};
