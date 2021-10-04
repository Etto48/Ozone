#include "include/hdd.h"

const uint16_t iBR = 0x01F0;
const uint16_t iCNL = 0x01F4;
const uint16_t iCNH = 0x01F5;
const uint16_t iSNR = 0x01F3;
const uint16_t iHND = 0x01F6;
const uint16_t iSCR = 0x01F2;
const uint16_t iERR = 0x01F1;
const uint16_t iCMD = 0x01F7;
const uint16_t iSTS = 0x01F7;
const uint16_t iDCR = 0x03F6;

void ack()
{
	io::inb(iSTS);
}
void enable_irq()
{
	io::outb(iDCR, 0x00);
}
void disable_irq()
{
    io::outb(iDCR, 0x02);
}
void wait_for_br()
{
	uint8_t s;
	do
		s = io::inb(iSTS);
	while (s & 0x88 != 0x08);
}
void input_sector(uint8_t* data)
{
	wait_for_br();
	auto p = (uint16_t*)data;
	for(uint64_t i = 0; i<256; i++)
	{
		p[i]=io::inw(iBR);
	}
}
void output_sector(uint8_t* data)
{
	wait_for_br();
	auto p = (uint16_t*)data;
	for(uint64_t i = 0; i<256; i++)
	{
		io::outw(iBR,p[i]);
	}
}
void set_lba(uint32_t lba)
{
	uint8_t 
        lba_0 = lba,
	    lba_1 = lba >> 8,
	    lba_2 = lba >> 16,
	    lba_3 = lba >> 24;

	io::outb(iSNR, lba_0);
	io::outb(iCNL, lba_1);
	io::outb(iCNH, lba_2);
	uint8_t hnd = (lba_3 & 0x0F) | 0xE0;
	io::outb(iHND, hnd);
}
void start_cmd(uint32_t lba, uint8_t n, uint8_t cmd)
{
	set_lba(lba);
	io::outb(iSCR,n);
	io::outb(iCMD,cmd);
}


int main()
{
    
    while (true)
    {
    }
}