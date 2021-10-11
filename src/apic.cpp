#include "include/apic.h"

namespace apic
{
    volatile uint32_t *EOIR = reinterpret_cast<volatile uint32_t *>(0xB0);
    volatile uint32_t *IOREGSEL = reinterpret_cast<uint32_t *>(0x00);
    volatile uint32_t *IOWIN = reinterpret_cast<uint32_t *>(0x10);

    volatile uint32_t *ESR = reinterpret_cast<uint32_t *>(0x280);
    volatile uint32_t *ICRL = reinterpret_cast<uint32_t *>(0x300);
    volatile uint32_t *ICRH = reinterpret_cast<uint32_t *>(0x310);

    bool PIC8259_compatibility_mode = false;
    void init()
    {
        //tty::color_write("PIIX3 not found\n",tty::term_color{vga::color::red,vga::color::black});
        //tty::write("Setting up compatibility mode...\n");
        PIC8259_compatibility_mode = true;
        apic::enable_8259();
        apic::remap_8259();
    }
    void set_lapic_base(void* lapicbase)
    {
        EOIR = (uint32_t*)((uint64_t)EOIR + (uint64_t)lapicbase);
        ICRL = (uint32_t*)((uint64_t)ICRL + (uint64_t)lapicbase);
        ICRH = (uint32_t*)((uint64_t)ICRH + (uint64_t)lapicbase);
        ESR = (uint32_t*)((uint64_t)ESR + (uint64_t)lapicbase);
    }
    void to_ioapic(void* ioapicbase,void* lapicbase)
    {
        IOWIN = (uint32_t*)((uint64_t)IOWIN + (uint64_t)ioapicbase);
        IOREGSEL = (uint32_t*)((uint64_t)IOREGSEL + (uint64_t)ioapicbase);
        set_lapic_base(lapicbase);
        PIC8259_compatibility_mode = false;
        apic::disable_8259();
        apic::reset();
    }
    void reset()
    {
        if (!PIC8259_compatibility_mode)
        {
            for (uint64_t i = 0; i < IRQ_MAX; i++)
            {
                apic::write_rth(i, 0);
                apic::write_rtl(i, 0);
            }
            for(uint8_t i = 0;i<16;i++)
            {
                apic::set_VECT(i,32+i);
            }
        }
    }

    uint32_t in(uint8_t off)
    {
        *apic::IOREGSEL = off;
        return *apic::IOWIN;
    }
    void out(uint8_t off, uint32_t v)
    {
        *apic::IOREGSEL = off;
        *apic::IOWIN = v;
    }

    uint32_t read_rth(uint8_t irq)
    {
        return apic::in(apic::RTO + irq * 2 + 1);
    }
    uint32_t read_rtl(uint8_t irq)
    {
        return apic::in(apic::RTO + irq * 2);
    }

    void write_rth(uint8_t irq, uint32_t w)
    {
        apic::out(RTO + irq * 2 + 1, w);
    }
    void write_rtl(uint8_t irq, uint32_t w)
    {
        apic::out(RTO + irq * 2, w);
    }

    void set_DELM(uint8_t irq, uint8_t delm)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rth(irq);
            work = (work & ~apic::DELM_MSK) | (delm << apic::DELM_SHF);
            apic::write_rth(irq, work);
        }
    }
    void set_DSTM(uint8_t irq, bool v)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rtl(irq);
            work = v ? (work | DSTM_BIT) : (work & ~DSTM_BIT);
            apic::write_rtl(irq, work);
        }
    }
    void set_DEST(uint8_t irq, uint8_t dest)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rth(irq);
            work = (work & ~apic::DEST_MSK) | (dest << apic::DEST_SHF);
            apic::write_rth(irq, work);
        }
    }
    void set_IPOL(uint8_t irq, bool v)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rtl(irq);
            work = v ? (work | IPOL_BIT) : (work & ~IPOL_BIT);
            apic::write_rtl(irq, work);
        }
    }
    void set_MIRQ(uint8_t irq, bool v)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rtl(irq);
            work = v ? (work | MIRQ_BIT) : (work & ~MIRQ_BIT);
            apic::write_rtl(irq, work);
        }
        else
        {
            uint16_t port;
            uint8_t value;
            if (v)
            {
                if (irq < 8)
                {
                    port = PIC8259_MASTER_DATA;
                }
                else
                {
                    port = PIC8259_SLAVE_DATA;
                    irq -= 8;
                }
                value = io::inb(port) | (1 << irq);
                io::outb(port, value);
            }
            else
            {
                if (irq < 8)
                {
                    port = PIC8259_MASTER_DATA;
                }
                else
                {
                    port = PIC8259_SLAVE_DATA;
                    irq -= 8;
                }
                value = io::inb(port) & ~(1 << irq);
                io::outb(port, value);
            }
        }
    }
    void set_TRGM(uint8_t irq, bool v)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rtl(irq);
            work = v ? (work | TRGM_BIT) : (work & ~TRGM_BIT);
            apic::write_rtl(irq, work);
        }
    }
    void set_VECT(uint8_t irq, uint8_t vec)
    {
        if (!PIC8259_compatibility_mode)
        {
            auto work = apic::read_rtl(irq);
            work &= 0xFFFFFF00;
            work |= vec;
            apic::write_rtl(irq, work);
        }
    }

    uint8_t get_bspid()
    {
        uint8_t bspid = 0;
        asm volatile ("mov $1, %%eax; cpuid; shrl $24, %%ebx;": "=b"(bspid) : : );
        return bspid;
    }
    void send_INIT(uint8_t lapic_id)
    {
        *ESR = 0;
        *ICRH = (*ICRH & 0x00ffffff) | (lapic_id << 24);         // select AP
	    *ICRL = (*ICRL & 0xfff00000) | 0x00C500;          // trigger INIT IPI
        do { 
            asm volatile ("pause" : : : "memory"); 
        }while(*ICRL & (1 << 12));         // wait for delivery
        //DEASSERT
        *ICRH = (*ICRH & 0x00ffffff) | (lapic_id << 24);         // select AP
	    *ICRL = (*ICRL & 0xfff00000) | 0x008500; 
        do { 
            asm volatile ("pause" : : : "memory"); 
        }while(*ICRL & (1 << 12));         // wait for delivery
    }
	void send_SIPI(uint8_t lapic_id,uint32_t starting_page_number)
    {
        *ESR = 0;
        *ICRH = (*ICRH & 0x00ffffff) | (lapic_id << 24);         // select AP
	    *ICRL = (*ICRL & 0xfff0f800) | starting_page_number;          // trigger STARTUP IPI for 0800:0000
        asm volatile("sti");
        clock::mwait(1);
        asm volatile("cli");
        do { 
            asm volatile ("pause" : : : "memory"); 
        }while(*ICRL & (1 << 12));         // wait for delivery
    }

    constexpr uint64_t ISR_SIZE = 32;
    void remap_8259()
    {

        constexpr uint8_t ICW1_ICW4 = 0x01; /* ICW4 (not) needed */
        //constexpr uint8_t ICW1_SINGLE=0x02;		/* Single (cascade) mode */
        //constexpr uint8_t ICW1_INTERVAL4=0x04;		/* Call address interval 4 (8) */
        //constexpr uint8_t ICW1_LEVEL=0x08;		/* Level triggered (edge) mode */
        constexpr uint8_t ICW1_INIT = 0x10; /* Initialization - required! */

        constexpr uint8_t ICW4_8086 = 0x01; /* 8086/88 (MCS-80/85) mode */
        //constexpr uint8_t ICW4_AUTO=0x02;		/* Auto (normal) EOI */
        //constexpr uint8_t ICW4_BUF_SLAVE=0x08;		/* Buffered mode/slave */
        //constexpr uint8_t ICW4_BUF_MASTER=0x0C;		/* Buffered mode/master */
        //constexpr uint8_t ICW4_SFNM=0x10;		/* Special fully nested (not) */
        unsigned char a1, a2;

        a1 = io::inb(PIC8259_MASTER_DATA); // save masks
        a2 = io::inb(PIC8259_SLAVE_DATA);

        io::outb(PIC8259_MASTER_COMMAND, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
        io::outb(PIC8259_SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);
        io::outb(PIC8259_MASTER_DATA, ISR_SIZE);    // ICW2: Master PIC vector offset
        io::outb(PIC8259_SLAVE_DATA, ISR_SIZE + 8); // ICW2: Slave PIC vector offset
        io::outb(PIC8259_MASTER_DATA, 4);           // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
        io::outb(PIC8259_SLAVE_DATA, 2);            // ICW3: tell Slave PIC its cascade identity (0000 0010)

        io::outb(PIC8259_MASTER_DATA, ICW4_8086);
        io::outb(PIC8259_SLAVE_DATA, ICW4_8086);

        io::outb(PIC8259_MASTER_DATA, a1); // restore saved masks.
        io::outb(PIC8259_SLAVE_DATA, a2);
    }

    void send_EOI(uint8_t intn)
    {
        if (!PIC8259_compatibility_mode)
        {
            *EOIR = 0;
        }
        else
        {
            constexpr uint8_t PIC_EOI = 0x20;
            if (intn >= ISR_SIZE + 8)
                io::outb(PIC8259_SLAVE_COMMAND, PIC_EOI);
            io::outb(PIC8259_MASTER_COMMAND, PIC_EOI);
        }
    }
};