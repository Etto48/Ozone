#include "include/debug.h"

namespace debug
{
	constexpr uint16_t iRBR = 0x03F8; // DLAB deve essere 0
	constexpr uint16_t iTHR = 0x03F8; // DLAB deve essere 0
	constexpr uint16_t iLSR = 0x03FD;
	constexpr uint16_t iLCR = 0x03FB;
	constexpr uint16_t iDLR_LSB = 0x03F8; // DLAB deve essere 1
	constexpr uint16_t iDLR_MSB = 0x03F9; // DLAB deve essere 1
	constexpr uint16_t iIER = 0x03F9;	  // DLAB deve essere 0
	constexpr uint16_t iMCR = 0x03FC;
	constexpr uint16_t iIIR = 0x03FA;

	void serial_o(uint8_t c)
	{
		uint8_t s;
		do
		{
			s = io::inb(iLSR);
		} while (!(s & 0x20));
		io::outb(iTHR, c);
	}

	void do_log(level lev, const char *buf, size_t size)
	{
		const char *level_text[] = {"INF", "WRN", "ERR"};
		uint8_t lev_sel = 3;
		switch (lev)
		{
		case level::err:
			lev_sel = 2;
			break;
		case level::wrn:
			lev_sel = 1;
			break;
		case level::inf:
			lev_sel = 0;
			break;
		default:
			break;
		}
		const uint8_t *l = (uint8_t *)level_text[lev_sel];
		while (*l)
			serial_o(*l++);
		serial_o((uint8_t)'\t');
		uint8_t idbuf[10];
		auto& ei = multitasking::current_execution_index();
		if (ei == multitasking::MAX_PROCESS_NUMBER)
		{
			serial_o((uint8_t)'-');
		}
		else
		{
			snprintf((char *)idbuf, 10, "%d", ei);
			l = idbuf;
			while (*l)
				serial_o(*l++);
		}
		serial_o((uint8_t)'\t');

		for (uint32_t i = 0; i < size; i++)
			serial_o(buf[i]);
		serial_o((uint8_t)'\n');
	}
	DECLARE_LOCK(debug_lock);
	void log(level lev, const char *fmt, ...)
	{
		LOCK(debug_lock);
		va_list ap;
		const uint32_t LOG_MSG_SIZE = 256;
		char buf[LOG_MSG_SIZE];

		va_start(ap, fmt);
		int l = vsnprintf(buf, LOG_MSG_SIZE, fmt, ap);
		va_end(ap);

		if (l > 1)
			do_log(lev, buf, l - 1);
		UNLOCK(debug_lock);
	}
	void init()
	{
		uint16_t CBITR = 0x000C; // 9600 bit/sec.
		io::outb(iLCR, 0x80);	 // DLAB 1
		io::outb(iDLR_LSB, CBITR);
		io::outb(iDLR_MSB, CBITR >> 8);
		io::outb(iLCR, 0x03); // 1 bit STOP, 8 bit/car, parita dis, DLAB 0
		io::outb(iIER, 0x00); // richieste di interruzione disabilitate
		io::inb(iRBR);		  // svuotamento buffer RBR
	}
};