#pragma once
#include <stdint.h>
#include <io.h>

namespace clock
{
	void mwait(uint64_t milliseconds);
};

namespace apic
{
    const uint32_t DEST_MSK = 0xFF000000; // destination field mask
	const uint32_t DEST_SHF = 24;         // destination field shift
	const uint32_t MIRQ_BIT = (1U << 16); // mask irq bit
	const uint32_t TRGM_BIT = (1U << 15); // trigger mode (1=level, 0=edge)
	const uint32_t IPOL_BIT = (1U << 13); // interrupt polarity (0=high, 1=low)
	const uint32_t DSTM_BIT = (1U << 11); // destination mode (0=physical, 1=logical)
	const uint32_t DELM_MSK = 0x00000700; // delivery mode field mask (000=fixed)
	const uint32_t DELM_SHF = 8;          // delivery mode field shift
	const uint32_t VECT_MSK = 0x000000FF; // vector field mask
	const uint32_t VECT_SHF = 0;          // vector field shift

	const uint32_t RTO = 0x10;
	const uint32_t IRQ_MAX = 24;

	const uint8_t PIC8259_MASTER_COMMAND = 0x20;
	const uint8_t PIC8259_MASTER_DATA = 0x21;
	const uint8_t PIC8259_SLAVE_COMMAND = 0xA0;
	const uint8_t PIC8259_SLAVE_DATA = 0xA1;

	extern bool PIC8259_compatibility_mode;

	extern volatile uint32_t* IOREGSEL;
	extern volatile uint32_t* IOWIN;

    extern volatile uint32_t* EOIR;
	extern volatile uint32_t* ICRL;
	extern volatile uint32_t* ICRH;

    void init();
	void to_ioapic(void* ioapicbase,void* lapicbase);
	void set_lapic_base(void* lapicbase);
    void reset();

    uint32_t in(uint8_t off);
    void out(uint8_t off, uint32_t v);

    uint32_t read_rth(uint8_t irq);
    uint32_t read_rtl(uint8_t irq);

    void write_rth(uint8_t irq,uint32_t w);
    void write_rtl(uint8_t irq,uint32_t w);
    
	void set_DELM(uint8_t irq, uint8_t delm);
	void set_DSTM(uint8_t irq, bool v);
    void set_DEST(uint8_t irq, uint8_t dest);
    void set_IPOL(uint8_t irq, bool v);
	void set_MIRQ(uint8_t irq, bool v);
	void set_TRGM(uint8_t irq, bool v);
	void set_VECT(uint8_t irq, uint8_t vec);

	uint8_t get_bspid();
	void send_INIT(uint8_t lapic_id);
	void send_SIPI(uint8_t lapic_id,uint32_t starting_page_number);

	namespace ipi
	{
		namespace dest_mode
		{
			enum destination_mode_t
			{
				normal = 0,
				lowest_priority = 1,
				SMI = 2,
				NMI = 4,
				INIT = 5,
				SIPI = 6
			};
		};
		namespace dest_type
		{
			enum destination_type_t
			{
				normal = 0,
				self = 1,
				broadcast = 2,
				broadcast_not_self = 3
			};
		};
	};
	void send_IPI(uint8_t target_lapic_id,uint8_t vector_number,ipi::dest_mode::destination_mode_t destination_mode,bool logical_destination,bool init_level_deassert,ipi::dest_type::destination_type_t destination_type);
	
	void remap_8259();

	extern "C" void disable_8259();
    extern "C" void enable_8259();
    void send_EOI(uint8_t intn);
};