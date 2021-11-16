OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

KERNEL_LMA = 0x0000000000200000; /* 2 MEG */
KERNEL_VMA = 0xffff800000000000; /* + KERNEL_LMA;*/

SECTIONS
{
    . = KERNEL_LMA;


 .boottext :
    {
      boot.o (.multiboot)
      boot.o (.text)
    }
  .bootrodata :
    {
      boot.o (.rodata)
    }
  .bootdata :
    {
      boot.o (.data)
    }
  .bootbss :
    {
      boot.o (.bss)
      boot.o (COMMON)
    }

	. = ALIGN(0x1000);
	_boot_end = .;

    . += KERNEL_VMA;


    .text : AT(_boot_end) {
        *(.text)
    }

    . = ALIGN(0x1000);

    .rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata)
    }

    . = ALIGN(0x1000);


    .data : AT(ADDR(.data) - KERNEL_VMA) {
        *(.data)
    }

    . = ALIGN(0x1000);

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) {
	_eh_frame = .;
	*(.eh_frame)
	*(.eh_frame_hdr)
	. = ALIGN(0x1000);
    }

    _edata = .;

    .bss : AT(ADDR(.bss) - KERNEL_VMA) {
        *(.bss)

	*(COMMON)
	. = ALIGN(0x1000);
    }

	PROVIDE(_end = .);
}
