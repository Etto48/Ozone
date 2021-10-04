ENTRY(_start)
SECTIONS
{
    KERNEL_LMA = 1M;
    KERNEL_VMA = 0x0;

    . = KERNEL_LMA;

    .bootstrap :
    {
        obj/main.s.o (.text)
    }

    . += KERNEL_VMA;

    .init_array : AT(ADDR(.init_array) - KERNEL_VMA)
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
	}

    .text : AT(ADDR(.text) - KERNEL_VMA)
    {   
        _code = .;
        *(EXCLUDE_FILE(*obj/main.s.o) .text)
        *(.rodata*)
        . = ALIGN(4096);
    }

   .data : AT(ADDR(.data) - KERNEL_VMA)
   {
        _data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA)
   {
       _ehframe = .;
       *(.eh_frame)
        . = ALIGN(4096);
   }

    _edata = .;

   .bss : AT(ADDR(.bss) - KERNEL_VMA)
   {
       _bss = .;
       *(.bss)

       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
       . = ALIGN(4096);
   }

   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}