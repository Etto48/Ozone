ENTRY(_start)
SECTIONS
{
    MODULE_VMA = 0xFFFF800000000000;
    . = MODULE_VMA;

    .text : ALIGN(4096)
    {   
        _code = .;


        PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(SORT(.preinit_array.*)))
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);

        PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);

        PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);

        *(.rodata*)
    }

   .data : ALIGN(4096)
   {
        _data = .;
        *(.data)
   }

   .eh_frame : ALIGN(4096)
   {
       _ehframe = .;
       *(.eh_frame)
   }

    

   .bss : ALIGN(4096)
   {
       _bss = .;
       *(.bss)

       /*
        * You usually need to include generated COMMON symbols
        * under kernel BSS section or use gcc's -fno-common
        */

        *(COMMON)
   }
   _edata = .;
   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}