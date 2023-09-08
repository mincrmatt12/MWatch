/*
 * Main MWatch bluetooth kernel linker script for the RP2040
 */

/*
 * Define usable region of flash / ram
 */

MEMORY
{
  FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 2048k
  RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
}

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapBase
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
	PROVIDE(__InitialMSPTop = ORIGIN(RAM) + LENGTH(RAM));

    .boot2 : {
        __boot2_start__ = .;
        KEEP (*(.boot2))
        __boot2_end__ = .;
    } > FLASH

    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);

        __text_end = .;
		PROVIDE(__etext = __text_end);
		PROVIDE(_etext = __text_end);
		PROVIDE(etext = __text_end);
    } > FLASH

	.rodata : 
	{
        *(.rodata)
        *(.rodata*)
	} > FLASH

	.ARM : 
	{
	  PROVIDE(__exidx_start = .);
	  PROVIDE(__exidx_end = .);
	} >FLASH   /* we provide our own exception handling stuff */

	.preinit_array :
	{
		. = ALIGN(8);

		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} >FLASH
	.init_array :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
	} >FLASH
	.fini_array :
	{
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} >FLASH

	.data :  {
		*(.data .data.*)
		*(.gnu.linkonce.d.*)

		/* Need to pre-align so that the symbols come after padding */
		. = ALIGN(8);
	} >RAM AT>FLASH
	PROVIDE(__data_start = ADDR(.data));
	PROVIDE(__data_source = LOADADDR(.data));

	/* Thread local initialized data. This gets
	 * space allocated as it is expected to be placed
	 * in ram to be used as a template for TLS data blocks
	 * allocated at runtime. We're slightly abusing that
	 * by placing the data in flash where it will be copied
	 * into the allocate ram addresses by the existing
	 * data initialization code in crt0
	 */
	.tdata :  {
		*(.tdata .tdata.* .gnu.linkonce.td.*)
		PROVIDE(__data_end = .);
		PROVIDE(__tdata_end = .);
	} >RAM AT>FLASH
	PROVIDE( __tls_base = ADDR(.tdata));
	PROVIDE( __tdata_start = ADDR(.tdata));
	PROVIDE( __tdata_source = LOADADDR(.tdata) );
	PROVIDE( __tdata_source_end = LOADADDR(.tdata) + SIZEOF(.tdata) );
	PROVIDE( __data_source_end = __tdata_source_end );
	PROVIDE( __tdata_size = SIZEOF(.tdata) );
	PROVIDE( __tls_align = MAX(ALIGNOF(.tdata),ALIGNOF(.tbss)) );

	PROVIDE( __edata = __data_end );
	PROVIDE( _edata = __data_end );
	PROVIDE( edata = __data_end );
	PROVIDE( __data_size = __data_end - __data_start );
	PROVIDE( __data_source_size = __data_source_end - __data_source );

	.tbss (NOLOAD) : {
		*(.tbss .tbss.* .gnu.linkonce.tb.*)
		*(.tcommon)
		PROVIDE( __tls_end = . );
		PROVIDE( __tbss_end = . );
	} >RAM
	PROVIDE( __bss_start = ADDR(.tbss));
	PROVIDE( __tbss_start = ADDR(.tbss));
	PROVIDE( __tbss_offset = ADDR(.tbss) - ADDR(.tdata) );
	PROVIDE( __tbss_size = SIZEOF(.tbss) );
	PROVIDE( __tls_size = __tls_end - __tls_base );
	PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
	PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );

	.bss (NOLOAD) : {
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)

		/* Align the heap */
		. = ALIGN(8);
		__bss_end = .;
	} >RAM
	PROVIDE( __non_tls_bss_start = ADDR(.bss) );
	PROVIDE( __end = __bss_end );
	PROVIDE( _end = __bss_end );
	PROVIDE( end = __bss_end );
	PROVIDE( __bss_size = __bss_end - __bss_start );

	/* Make the rest of memory available for heap storage */
	PROVIDE (__heap_start = __end);
	PROVIDE (__heap_end = __InitialMSPTop - 0x1000);
	PROVIDE (__heap_size = __heap_end - __heap_start);

	/DISCARD/ : {
		*(.note .note.*)
		*(.eh_frame .eh_frame.*)
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		*(.ARM.exidx*)
	}
}
